// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "MFItemTypes.h"
#include "MFInventoryComponent.generated.h"

class UMFItemDatabase;
class UDataTable;
class AMFPetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPetRosterChanged);
/** 任意宠物复活读秒推进时广播（每秒一次）。UI 据此刷新读秒数字，无需重建槽位。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPetReviveTick);

/**
 * UMFInventoryComponent — 玩家背包组件。
 *
 * 管理两类运行时数据：
 *   ResourceSlots — 可叠加资源（木材、矿石等）。
 *   PetSlots      — 宠物实例列表（含成长数据与属性快照）。
 *
 * 宠物生命周期：
 *   捕获 → RegisterCaughtPet()  →  PetSlots 新增条目，Actor 由 GA_CatchPet 销毁
 *   召唤 → SummonPet()          →  SpawnActor + RestoreFromInstance，写入 ActivePetActors
 *   召回 → RecallPet()          →  SerializeToInstance 刷新快照，Actor 销毁
 */
UCLASS(ClassGroup = (MF), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFInventoryComponent();

	// -----------------------------------------------------------------------
	// 配置（由 AMFCharacter::BeginPlay 从 PlayerConfig 注入，勿在此处直接设置）
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Config")
	TObjectPtr<UMFItemDatabase> ItemDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Config")
	TObjectPtr<UDataTable> AIRegistry;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Config")
	int32 MaxResourceSlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Config")
	int32 MaxPetSlots = 0;

	/** 召唤宠物的出生阵营标签（由 PlayerConfig 注入）。SummonPet 时写入宠物 ASC。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Config")
	FGameplayTagContainer SummonedPetTeamTags;

	/** 召唤宠物的索敌阵营标签（由 PlayerConfig 注入）。SummonPet 时覆盖宠物雷达 TargetTags。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Config")
	FGameplayTagContainer SummonedPetTargetTags;

	// -----------------------------------------------------------------------
	// 广播
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnPetRosterChanged OnPetRosterChanged;

	/** 复活读秒每秒推进时广播，供 UI 刷新倒计时显示。状态切换（死亡/复活完成）仍走 OnPetRosterChanged。 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnPetReviveTick OnPetReviveTick;

	// -----------------------------------------------------------------------
	// 资源接口
	// -----------------------------------------------------------------------

	/** 添加资源，相同 ItemID 自动叠加。返回实际添加数量。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Resource")
	int32 AddResource(FName ItemID, int32 Count);

	/** 移除资源。数量不足时不做任何修改，返回 false。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Resource")
	bool RemoveResource(FName ItemID, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Inventory|Resource")
	int32 GetResourceCount(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Resource")
	bool HasResource(FName ItemID, int32 Count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Resource")
	const TArray<FMFInventorySlot>& GetResourceSlots() const { return ResourceSlots; }

	// -----------------------------------------------------------------------
	// 宠物接口
	// -----------------------------------------------------------------------

	/**
	 * 注册捕获的宠物。由 GA_CatchPet 在 SerializeToInstance 后调用。
	 * 自动生成 InstanceID、从 DA 读取默认昵称、设置 Level=1 Exp=0。
	 * @return  新实例 ID；FGuid() 表示失败。
	 */
	FGuid RegisterCaughtPet(const FMFPetInstance& Snapshot);

	/**
	 * 在指定位置召唤宠物 Actor，并从 PetSlots 快照还原属性。
	 * 由 GA_SummonPet 调用（GAS Ability，待实现）。
	 * @return  成功返回 Actor 指针，失败返回 nullptr。
	 */
	AMFPetBase* SummonPet(FGuid InstanceID, FVector Location);

	/**
	 * 召回出战宠物：刷新 PetSlots 快照后销毁 Actor。
	 * 由 GA_SummonPet 或其他召唤管理逻辑调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Pet")
	void RecallPet(FGuid InstanceID);

	/**
	 * 设置宠物阵亡后的复活读秒时长（秒）。由 AMFGameMode 从 GameLoopConfig 注入。
	 * 0 表示阵亡后立即可再召唤。
	 */
	void SetPetReviveDuration(float Seconds) { PetReviveDuration = FMath::Max(0.f, Seconds); }

	/** 通过 InstanceID 查找宠物实例（只读，C++ 专用）。 */
	const FMFPetInstance* FindPet(FGuid InstanceID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Pet")
	const TArray<FMFPetInstance>& GetAllPets() const { return PetSlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Pet")
	TArray<FMFPetInstance> GetActivePets() const;

	/** 返回当前在世界中存活的所有宠物 Actor（过滤已失效指针）。供 GameLoopManager 等系统使用。 */
	TArray<AMFPetBase*> GetActivePetActors() const;

	/** 按 InstanceID 反查出战中的宠物 Actor；未召唤或已销毁返回 nullptr。供 UI 绑定实时血条。 */
	AMFPetBase* GetActivePetActor(FGuid InstanceID) const;

private:
	// -----------------------------------------------------------------------
	// 运行时数据
	// -----------------------------------------------------------------------

	UPROPERTY()
	TArray<FMFInventorySlot> ResourceSlots;

	UPROPERTY()
	TArray<FMFPetInstance> PetSlots;

	/** 当前在世界中存活的宠物 Actor（弱引用，Actor 意外销毁时自动失效）。 */
	TMap<FGuid, TWeakObjectPtr<AMFPetBase>> ActivePetActors;

	// -----------------------------------------------------------------------
	// 复活
	// -----------------------------------------------------------------------

	/** 宠物阵亡后的复活读秒时长（秒），由 GameMode 注入。 */
	float PetReviveDuration = 5.f;

	/** 复活读秒计时器（1s 循环），仅在至少一只宠物复活中时运行。 */
	FTimerHandle ReviveTickHandle;

	/** 出战宠物阵亡回调（订阅其 AttributeSet.OnDeath，绑定时附带 InstanceID）。 */
	void HandlePetDied(FGuid InstanceID);

	/** 复活读秒每秒推进；归零的宠物回满血并清除死亡态；无人复活时停止计时器。 */
	void OnReviveTick();

	/** 启动复活计时器（若尚未运行）。 */
	void EnsureReviveTickerRunning();

	/** 让单只宠物立即完成复活：清死亡态、把快照 Health 设回 MaxHealth。 */
	void ReviveSinglePet(FMFPetInstance& Instance) const;

	/** 下一帧销毁出战宠物 Actor（避免在 OnDeath 广播过程中销毁自身 ASC）。 */
	void DestroyPetActorDeferred(FGuid InstanceID);

	// -----------------------------------------------------------------------
	// 辅助
	// -----------------------------------------------------------------------

	int32 FindResourceSlotIndex(FName ItemID) const;

	/** 返回可修改的宠物实例指针，未找到返回 nullptr。 */
	FMFPetInstance* FindPetMutable(FGuid InstanceID);
};

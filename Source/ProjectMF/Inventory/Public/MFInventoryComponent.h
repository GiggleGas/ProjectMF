// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFItemTypes.h"
#include "MFInventoryComponent.generated.h"

class UMFItemDatabase;
class AMFPetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPetRosterChanged);

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
	// 配置
	// -----------------------------------------------------------------------

	/** 全局物品数据库，在 BP_MFCharacter 的组件 Details 中赋值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Config")
	TObjectPtr<UMFItemDatabase> ItemDatabase;

	/** 资源格子上限（0 = 不限制）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = 0))
	int32 MaxResourceSlots = 0;

	/** 宠物携带上限（0 = 不限制）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = 0))
	int32 MaxPetSlots = 0;

	// -----------------------------------------------------------------------
	// 广播
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnPetRosterChanged OnPetRosterChanged;

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

	/** 通过 InstanceID 查找宠物实例（只读，C++ 专用）。 */
	const FMFPetInstance* FindPet(FGuid InstanceID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Pet")
	const TArray<FMFPetInstance>& GetAllPets() const { return PetSlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Pet")
	TArray<FMFPetInstance> GetActivePets() const;

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
	// 辅助
	// -----------------------------------------------------------------------

	int32 FindResourceSlotIndex(FName ItemID) const;

	/** 返回可修改的宠物实例指针，未找到返回 nullptr。 */
	FMFPetInstance* FindPetMutable(FGuid InstanceID);
};

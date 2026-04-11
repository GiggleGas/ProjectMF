// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFItemTypes.h"
#include "MFInventoryComponent.generated.h"

class UMFItemDatabase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPetRosterChanged);

/**
 * UMFInventoryComponent — 玩家背包组件。
 *
 * 挂载到 AMFCharacter 上，管理两类运行时数据：
 *
 *   ResourceSlots — 资源格子（可叠加，相同 ItemID 自动合并）。
 *   PetSlots      — 宠物实例列表（每只宠物独立，含成长数据）。
 *
 * 使用方式：
 *   1. 在 BP_MFCharacter 的 InventoryComponent Details 中将 DA_ItemDatabase 赋给 ItemDatabase。
 *   2. 监听 OnInventoryChanged / OnPetRosterChanged 更新 UI。
 *   3. 捕获成功后调用 AddPet(PetItemID)，拾取资源调用 AddResource(ItemID, Count)。
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

	/** 资源格子变化时广播（UI 监听此事件刷新）。 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	/** 宠物名册变化时广播。 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnPetRosterChanged OnPetRosterChanged;

	// -----------------------------------------------------------------------
	// 资源接口
	// -----------------------------------------------------------------------

	/**
	 * 添加资源到背包。相同 ItemID 自动叠加，超出 MaxStackSize 的溢出部分丢弃。
	 * @param ItemID   须在 ItemDatabase 中存在且 ItemType == Resource。
	 * @param Count    添加数量（> 0）。
	 * @return         实际添加的数量。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Resource")
	int32 AddResource(FName ItemID, int32 Count);

	/**
	 * 从背包移除指定数量资源。
	 * @return  true = 移除成功；false = 数量不足，不做任何修改。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Resource")
	bool RemoveResource(FName ItemID, int32 Count);

	/** 查询背包中指定物品的总数量。 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Resource")
	int32 GetResourceCount(FName ItemID) const;

	/** 检查是否拥有足够数量的资源。 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Resource")
	bool HasResource(FName ItemID, int32 Count = 1) const;

	/** 获取所有资源格子（只读）。 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Resource")
	const TArray<FMFInventorySlot>& GetResourceSlots() const { return ResourceSlots; }

	// -----------------------------------------------------------------------
	// 宠物接口
	// -----------------------------------------------------------------------

	/**
	 * 注册新捕获的宠物，生成并返回宠物实例 ID。
	 * @param PetItemID   对应 ItemDatabase 中的宠物定义 ID。
	 * @param PetName     初始昵称（空字符串时使用 DisplayName）。
	 * @return            新实例 ID；FGuid() 表示失败（名册已满或 ID 无效）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Pet")
	FGuid AddPet(FName PetItemID, const FString& PetName = TEXT(""));

	/**
	 * 通过 InstanceID 查找宠物实例（只读）。
	 * @return  找到返回指针，未找到返回 nullptr。（Blueprint 不可用，C++ 专用）
	 */
	const FMFPetInstance* FindPet(FGuid InstanceID) const;

	/** 设置宠物的出战状态。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Pet")
	void SetPetActive(FGuid InstanceID, bool bActive);

	/** 获取所有出战宠物列表（返回副本）。 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Pet")
	TArray<FMFPetInstance> GetActivePets() const;

	/** 获取所有宠物实例（只读）。 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Pet")
	const TArray<FMFPetInstance>& GetAllPets() const { return PetSlots; }

private:
	// -----------------------------------------------------------------------
	// 运行时数据（UPROPERTY 保证 GC 安全）
	// -----------------------------------------------------------------------

	UPROPERTY()
	TArray<FMFInventorySlot> ResourceSlots;

	UPROPERTY()
	TArray<FMFPetInstance> PetSlots;

	// -----------------------------------------------------------------------
	// 辅助
	// -----------------------------------------------------------------------

	/** 在 ResourceSlots 中找到 ItemID 对应的格子下标，未找到返回 INDEX_NONE。 */
	int32 FindResourceSlotIndex(FName ItemID) const;
};

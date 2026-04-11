// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFItemTypes.h"
#include "MFItemDatabase.generated.h"

/**
 * UMFItemDatabase — 全局物品数据库 DataAsset。
 *
 * 使用方式：
 *   1. 在编辑器中创建 DA_ItemDatabase（继承本类）。
 *   2. 在 Items 数组中填写所有物品定义，ItemID 须全局唯一。
 *   3. 在 BP_MFCharacter 的 InventoryComponent Details 中将本 DA 赋值给 ItemDatabase。
 *   4. 运行时通过 FindItem(ItemID) 查询定义（TMap 缓存，O(1) 查询）。
 *
 * ItemID 命名规范：
 *   Item.Resource.Wood / Item.Resource.Stone
 *   Item.Pet.SlimeCat  / Item.Pet.FireFox
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFItemDatabase : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 所有物品定义，在编辑器中填写。ItemID 须全局唯一，不可为空。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	TArray<FMFItemDef> Items;

	/**
	 * 根据 ItemID 查找物品定义（O(1)）。C++ 专用。
	 * @return  找到返回指针，未找到返回 nullptr。
	 */
	const FMFItemDef* FindItem(FName ItemID) const;

	/**
	 * 根据 ItemID 获取物品定义（Blueprint 可用）。
	 * @param OutDef  找到时填入物品定义。
	 * @return        true = 找到；false = 不存在。
	 */
	UFUNCTION(BlueprintCallable, Category = "Items")
	bool GetItem(FName ItemID, FMFItemDef& OutDef) const;

	/** 检查 ItemID 是否存在于数据库中。 */
	UFUNCTION(BlueprintPure, Category = "Items")
	bool ContainsItem(FName ItemID) const;

protected:
	/** 资产加载后构建查询缓存。 */
	virtual void PostLoad() override;

private:
	/** ItemID → Items 数组下标，PostLoad 时构建。 */
	TMap<FName, int32> LookupCache;

	void BuildLookupCache();
};

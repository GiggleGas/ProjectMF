// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPetBase.h"
#include "MFItemTypes.generated.h"

// ============================================================
// EMFItemType
// ============================================================

/** 物品类型枚举，决定背包的叠加规则和 UI 显示分类。 */
UENUM(BlueprintType)
enum class EMFItemType : uint8
{
	Resource   UMETA(DisplayName = "Resource"),    // 可叠加资源（木材、矿石等）
	Pet        UMETA(DisplayName = "Pet"),          // 宠物（不叠加，有成长数据）
	Equipment  UMETA(DisplayName = "Equipment"),    // 装备（预留）
	Consumable UMETA(DisplayName = "Consumable"),   // 消耗品（预留）
};

// ============================================================
// FMFItemDef
// ============================================================

/**
 * FMFItemDef — 物品静态定义，存储在 UMFItemDatabase DataAsset 中。
 *
 * 命名规范（ItemID）：
 *   资源  →  Item.Resource.Wood / Item.Resource.Stone
 *   宠物  →  Item.Pet.SlimeCat / Item.Pet.FireFox
 *   装备  →  Item.Equipment.Sword
 *
 * 运行时只读，不随游戏状态变化。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFItemDef
{
	GENERATED_BODY()

	/** 全局唯一 ID（主键），用于所有系统间的引用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EMFItemType ItemType = EMFItemType::Resource;

	/**
	 * 单格最大叠加数量。
	 * Pet / Equipment 固定应设为 1（不叠加）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 MaxStackSize = 99;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	/**
	 * 仅 ItemType == Pet 时有效：对应的宠物 Actor 类。
	 * 用于从背包召唤宠物（预留）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Pet")
	TSubclassOf<AMFPetBase> PetClass;
};

// ============================================================
// FMFInventorySlot
// ============================================================

/**
 * FMFInventorySlot — 背包资源格子（运行时）。
 * 仅存储可叠加资源类物品，宠物由 FMFPetInstance 数组单独管理。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFInventorySlot
{
	GENERATED_BODY()

	/** 引用 UMFItemDatabase 中的物品定义。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("[Resource] %s x%d"), *ItemID.ToString(), Count);
	}
};

// ============================================================
// FMFPetInstance
// ============================================================

/**
 * FMFPetInstance — 宠物运行时实例数据。
 *
 * 每只捕获的宠物对应一个独立实例：
 *   - PetItemID 指向 DA 中的静态定义（基础模板）
 *   - 其余字段为该宠物个体的成长数据
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFPetInstance
{
	GENERATED_BODY()

	/** 唯一实例 ID，用于跨系统精确引用这只宠物。 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet")
	FGuid InstanceID;

	/** 对应 UMFItemDatabase 中的宠物物品定义 ID（静态模板）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet")
	FName PetItemID;

	/** 玩家自定义昵称，默认使用 DisplayName。 */
	UPROPERTY(BlueprintReadWrite, Category = "Pet")
	FString PetName;

	UPROPERTY(BlueprintReadOnly, Category = "Pet")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Pet")
	int32 Experience = 0;

	/** 是否当前出战（上阵）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet")
	bool bIsActive = false;

	bool IsValid() const { return InstanceID.IsValid() && !PetItemID.IsNone(); }

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("[Pet] %s  Lv.%d  (%s)  [%s]"),
			*PetName, Level, *PetItemID.ToString(),
			bIsActive ? TEXT("Active") : TEXT("Stored"));
	}
};

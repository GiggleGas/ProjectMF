// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MFPetBase.h"
#include "MFItemTypes.generated.h"

class UPaperSprite;

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
 * FMFItemDef — 物品静态定义，作为物品总表 DataTable（DT_ItemDatabase）的行结构。
 *
 * 主键 = DataTable 的 RowName，用数字字符串（"1001"/"1002"…）；该数字即 ItemID。
 * 其他系统（背包 slot / 掉落表 / 配方）一律用 int32 ItemID 索引，经 UMFItemStatics 查表。
 *
 * 注意：宠物类物品不经过本表，由 DT_AIRegistry + UMFPetConfig 统一管理。
 * 运行时只读，不随游戏状态变化。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFItemDef : public FTableRowBase
{
	GENERATED_BODY()

	/** 显示名（RowName 是数字主键，可读名放这里）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	/** UI 图标（背包 / 合成界面用）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** 掉落到场景中的 2D 外观 Sprite（Paper2D）。掉落物 Actor 用它在场景展示。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UPaperSprite> WorldSprite = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EMFItemType ItemType = EMFItemType::Resource;

	/**
	 * 单格最大叠加数量。
	 * Equipment 固定应设为 1（不叠加）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 MaxStackSize = 99;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;
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

	/** 数字 ItemID（= 物品总表 DataTable 的 RowName）。0 = 空格。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", SaveGame)
	int32 ItemID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", SaveGame)
	int32 Count = 0;

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("[Resource] #%d x%d"), ItemID, Count);
	}
};

// ============================================================
// FMFPetInstance
// ============================================================

/**
 * FMFPetInstance — 宠物运行时实例数据。
 *
 * 每只捕获的宠物对应一个独立实例：
 *   - AIConfigID 对应 DT_AIRegistry DataTable 的 RowKey（全局 AI 类型 ID）
 *   - 其余字段为该宠物个体的成长数据
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFPetInstance
{
	GENERATED_BODY()

	/** 唯一实例 ID，用于跨系统精确引用这只宠物。 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	FGuid InstanceID;

	/**
	 * AI 类型 ID，对应 DT_AIRegistry DataTable 的 RowKey（如 "Pet_SlimeCat"）。
	 * 召唤时通过此 ID 从 DT 查找 UMFPetConfig，重新 ApplyPetConfig + RunStateTree。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	FName AIConfigID;

	/** 玩家自定义昵称，默认使用 UMFPetConfig::DisplayName。 */
	UPROPERTY(BlueprintReadWrite, Category = "Pet", SaveGame)
	FString PetName;

	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	int32 Experience = 0;

	/** 是否当前出战（上阵）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	bool bIsActive = false;

	/**
	 * 是否处于阵亡复活倒计时中。
	 * true 期间宠物已从战场移除、不可召唤；ReviveTimeRemaining 归零后清除并回满血。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	bool bIsDead = false;

	/** 复活剩余秒数（仅 bIsDead 为 true 时有效，供 UI 读秒显示）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Pet", SaveGame)
	float ReviveTimeRemaining = 0.f;

	/** 捕获时快照的关键属性值。Key = 属性名，Value = 捕获瞬间的值。 */
	UPROPERTY(SaveGame)
	TMap<FName, float> AttributeSnapshot;

	bool IsValid() const { return InstanceID.IsValid() && !AIConfigID.IsNone(); }

	FString GetDebugString() const
	{
		if (bIsDead)
		{
			return FString::Printf(TEXT("[Pet] %s  Lv.%d  (%s)  [Reviving %.0fs]"),
				*PetName, Level, *AIConfigID.ToString(), ReviveTimeRemaining);
		}
		return FString::Printf(TEXT("[Pet] %s  Lv.%d  (%s)  [%s]"),
			*PetName, Level, *AIConfigID.ToString(),
			bIsActive ? TEXT("Active") : TEXT("Stored"));
	}
};

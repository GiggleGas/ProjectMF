// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFLootTypes.generated.h"

// ============================================================
// FMFLootEntry
// ============================================================

/**
 * 掉落表中的一条目：每条独立判定（roll Chance → 命中后数量在 [CountMin, CountMax] 均匀取整）。
 * 饥荒式配法示例：肉 Chance=1.0 Count=1-2 + 皮 Chance=0.5 Count=1。
 *
 * 互斥权重组（N 选 1）暂不支持，有真实需求时在 UMFLootTable 上加 RollMode 扩展。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFLootEntry
{
	GENERATED_BODY()

	/** 掉落物品 ID，引用 UMFItemDatabase 定义（规范 Item.Resource.*）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	FName ItemID;

	/** 独立命中概率（0~1，1=必掉）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = 1))
	int32 CountMin = 1;

	/** 命中后数量上限（含）。小于 CountMin 时按 CountMin 处理。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = 1))
	int32 CountMax = 1;
};

// ============================================================
// FMFLootResult
// ============================================================

/** 一次 roll 的产出（同 ItemID 已合并）。UMFLootSubsystem::RollTable 输出、SpawnLoot 输入。 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFLootResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	FName ItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Count = 0;
};

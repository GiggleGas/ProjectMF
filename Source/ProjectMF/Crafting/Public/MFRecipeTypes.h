// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MFItemKey.h"
#include "MFRecipeTypes.generated.h"

/** 配方输入项：物品 + 数量。 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFItemCount
{
	GENERATED_BODY()

	/** 物品（编辑器下拉选择）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FMFItemKey Item;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = 1))
	int32 Count = 1;
};

/**
 * FMFRecipeDef — 合成配方，DT_RecipeLibrary 的行结构。RowName = 唯一配方 ID。
 *
 * Inputs 全满足才可合成；Craft 扣掉 Inputs、产出 Output × OutputCount。
 * Inputs/Output 用 FMFItemKey → 编辑器下拉选物品。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFRecipeDef : public FTableRowBase
{
	GENERATED_BODY()

	/** 需要消耗的资源（全部满足才可合成）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TArray<FMFItemCount> Inputs;

	/** 产出物品（下拉选，通常 Consumable 类）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FMFItemKey Output;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = 1))
	int32 OutputCount = 1;

	/** UI 显示名（留空回退产出物 DisplayName）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FText DisplayName;
};

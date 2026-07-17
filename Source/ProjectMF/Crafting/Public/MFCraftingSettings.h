// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MFCraftingSettings.generated.h"

class UDataTable;

/**
 * UMFCraftingSettings — 打造系统全局配置（Project Settings → Game → MF Crafting）。
 *
 * 配方库 DataTable 的唯一数据源（仿 UMFItemSettings 模式，不走 PlayerConfig 注入）。
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "MF Crafting"))
class PROJECTMF_API UMFCraftingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 配方库 DataTable（行结构 FMFRecipeDef，RowName = 配方 ID）。 */
	UPROPERTY(EditAnywhere, config, Category = "Crafting",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectMF.MFRecipeDef"))
	TSoftObjectPtr<UDataTable> RecipeLibrary;

	virtual FName GetCategoryName() const override { return FName("Game"); }

	/** 加载并返回配方库（未配置返回 nullptr）。全系统统一入口。 */
	static const UDataTable* GetRecipeTable();
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MFItemSettings.generated.h"

class UDataTable;

/**
 * UMFItemSettings — 物品系统全局配置（Project Settings → Game → MF Item，写入 DefaultGame.ini）。
 *
 * 物品总表 DataTable 的唯一数据源：背包 / 掉落 / 合成 都从这里取表，
 * 不再各自注入，避免多处配置漂移。
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "MF Item"))
class PROJECTMF_API UMFItemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 物品总表 DataTable（行结构 FMFItemDef，RowName = 数字 ItemID）。 */
	UPROPERTY(EditAnywhere, config, Category = "Item",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectMF.MFItemDef"))
	TSoftObjectPtr<UDataTable> ItemDatabase;

	virtual FName GetCategoryName() const override { return FName("Game"); }

	/** 加载并返回物品总表（未配置返回 nullptr）。全系统统一入口。 */
	static const UDataTable* GetItemTable();
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFLootTypes.h"
#include "MFLootTable.generated.h"

/**
 * UMFLootTable — 通用掉落表 DataAsset。
 *
 * 使用方式：
 *   1. 编辑器中创建资产（命名规范 LT_SlimeCat / LT_Tree_Oak / LT_Boss_XXX）。
 *   2. 填 Entries：每条独立 roll（见 FMFLootEntry）。
 *   3. 挂到 UMFAIConfig::LootTable（怪物死亡掉落）或 AMFGatherNode::LootTable（采集产出）。
 *
 * roll / 生成统一走 UMFLootSubsystem::DropFromTable，本资产纯数据。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 掉落条目，每条独立判定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FMFLootEntry> Entries;
};

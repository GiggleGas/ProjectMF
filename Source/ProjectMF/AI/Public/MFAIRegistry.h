// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MFAIRegistry.generated.h"

class UMFPetConfig;

/**
 * FMFAIRegistryRow — AI 全局注册表的行结构。
 *
 * 用于 DT_AIRegistry DataTable，以 FName RowKey 作为 AI 的全局唯一 ID（如 "Pet_SlimeCat"）。
 * 设计师在 DataTable 编辑器中为每种 AI 填写一行，指向对应的 UMFPetConfig DataAsset。
 *
 * 运行时查询示例：
 *   const FMFAIRegistryRow* Row = AIRegistry->FindRow<FMFAIRegistryRow>(AIConfigID, TEXT(""));
 *   UMFPetConfig* Config = Row ? Row->Config.LoadSynchronous() : nullptr;
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFAIRegistryRow : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * 指向该 AI 的配置 DataAsset（UMFPetConfig）。
	 * 软引用：资产不会随 DataTable 一起常驻内存，召唤时按需加载。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TSoftObjectPtr<UMFPetConfig> Config;
};

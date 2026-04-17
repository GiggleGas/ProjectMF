// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFSpawnAIConfig.generated.h"

class AMFPetBase;
class AMFPetAIController;
class UStateTree;

/**
 * UMFSpawnAIConfig — 单种宠物的生成配置（DataAsset）。
 *
 * 只负责"生什么"：
 *   - 哪个 Pet 蓝图类
 *   - 绑定哪个 StateTree
 *   - 使用哪个 AIController（可留空，默认 AMFPetAIController）
 *
 * "生多少"和"怎么选点"由 AMFSpawnAIManager 上的 FMFSpawnEntry 决定，
 * 同一份 Config 可被多个 Manager / 多条 Entry 复用。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFSpawnAIConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 要生成的 Pet 蓝图类（必须继承 AMFPetBase）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnAI")
	TSubclassOf<AMFPetBase> PetClass;

	/**
	 * 绑定的 StateTree 资产。
	 * 生成后由 Controller 调用 RunStateTree() 启动。
	 * 留空则 Pet 仅依赖默认 AIController 行为（无 StateTree）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnAI")
	TObjectPtr<UStateTree> StateTreeAsset;

	/**
	 * 使用的 AIController 类（必须继承 AMFPetAIController）。
	 * 留空则使用 AMFPetBase CDO 上设置的默认类（AMFPetAIController）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnAI")
	TSubclassOf<AMFPetAIController> ControllerClass;
};

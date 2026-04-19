// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
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

	/**
	 * 雷达感知配置：Spawn 完成后写入宠物的 UMFRadarSensingComponent。
	 *
	 * 每种宠物类型可独立设置：
	 *   SensingRadius  — 感知半径（cm）
	 *   TargetTags     — 目标阵营标签（通常为 MF.Team.Player）
	 *   ScanInterval   — 扫描频率（秒）
	 *
	 * 不填写时组件使用 CDO 中的默认值（Radius=1000, Interval=0.15s, Tags=空）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnAI|Radar")
	FMFRadarSensingConfig RadarConfig;

	/**
	 * 索敌配置：Spawn 完成后写入宠物的 UMFThreatComponent。
	 * RadarConfig 须先写入，ApplyConfig 内部会校验 EngagementRadius <= SensingRadius。
	 *
	 * 每种宠物类型可独立设置：
	 *   EngagementRadius — 交战半径（须 <= SensingRadius）
	 *   LockDuration     — 锁定持续时间（秒）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnAI|Threat")
	FMFThreatConfig ThreatConfig;
};

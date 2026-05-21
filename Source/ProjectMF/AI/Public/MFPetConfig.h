// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAIConfig.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFAIAnimInstance.h"
#include "MFPetConfig.generated.h"

class AMFPetBase;
class AMFPetAIController;
class UStateTree;

/**
 * UMFPetConfig — 单种宠物的完整配置（DataAsset）。
 *
 * 继承 UMFAIConfig，获得 GAS / OverheadWidget / HitFlash 通用配置。
 * 本类只添加宠物专属字段：
 *   - 生成：蓝图类 / AIController 类 / StateTree
 *   - 感知：雷达配置 / 索敌配置
 *   - 动画：AnimInstance 类
 *   - 身份：AIConfigID / DisplayName / Icon / Description
 *
 * 生成后通过 AMFPetBase::ApplyPetConfig() 将所有配置写入宠物 Actor。
 * 同一份 Config 可被多个 SpawnAIManager / FMFSpawnEntry 复用。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFPetConfig : public UMFAIConfig
{
	GENERATED_BODY()

public:
	// ----------------------------------------------------------------
	// 生成
	// ----------------------------------------------------------------

	/** 要生成的 Pet 蓝图类（必须继承 AMFPetBase）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AMFPetBase> PetClass;

	/**
	 * 使用的 AIController 类（必须继承 AMFPetAIController）。
	 * 留空则使用 AMFPetBase CDO 上的默认类（AMFPetAIController）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AMFPetAIController> ControllerClass;

	/**
	 * 绑定的 StateTree 资产，Spawn 后由 Controller 调用 RunStateTree() 启动。
	 * 留空则 Pet 仅依赖 AIController 默认行为（无 StateTree）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UStateTree> StateTreeAsset;

	// ----------------------------------------------------------------
	// 感知
	// ----------------------------------------------------------------

	/**
	 * 雷达感知配置：写入宠物的 UMFRadarSensingComponent。
	 * SensingRadius / TargetTags / ScanInterval。
	 * 不填时组件使用 CDO 默认值（Radius=1000, Interval=0.15s）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception")
	FMFRadarSensingConfig RadarConfig;

	/**
	 * 索敌配置：写入宠物的 UMFThreatComponent。
	 * EngagementRadius 须 <= SensingRadius（内部有校验）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception")
	FMFThreatConfig ThreatConfig;

	// ----------------------------------------------------------------
	// 动画
	// ----------------------------------------------------------------

	/**
	 * 使用的 AnimBP 类（必须继承 UMFAIAnimInstance）。
	 * Spawn 后通过 UPaperZDAnimationComponent::SetAnimInstanceClass() 替换当前实例。
	 *
	 * 典型工作流：
	 *   1. 在 Content Browser 中新建 PaperZD AnimBP，Parent = UMFAIAnimInstance（或其子类）。
	 *   2. 在 AnimBP 中设计状态机：读取 Speed / DirectionalInput 驱动 Idle / Walk 过渡。
	 *   3. 将该 AnimBP 类赋值给此字段。
	 *
	 * 留空则保留蓝图 CDO 上绑定的 AnimBP（非共用蓝图模式）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UMFAIAnimInstance> AnimInstanceClass;

	// ----------------------------------------------------------------
	// 身份 & 显示
	// ----------------------------------------------------------------

	/**
	 * 全局唯一 AI 配置 ID，对应 DT_AIRegistry DataTable 的 RowKey（如 "Pet_SlimeCat"）。
	 * 写入宠物的 AIConfigID，供捕获后序列化到 FMFPetInstance 使用。
	 * 留空（NAME_None）则保留蓝图 CDO 上的默认值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName AIConfigID;

	/** 宠物显示名称（UI / 背包 / 头顶标签使用）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** 宠物头像图标（背包卡槽 / HUD 使用）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 宠物描述文本（图鉴 / Tooltip 使用）。可选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText Description;

	// GAS、OverheadWidget、HitFlashDuration 均继承自 UMFAIConfig。
};

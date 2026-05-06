// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFAIAnimInstance.h"
#include "MFPetConfig.generated.h"

class AMFPetBase;
class AMFPetAIController;
class UStateTree;
class UMFGameplayAbilityBase;
class UGameplayEffect;

/**
 * UMFPetConfig — 单种宠物的完整配置（DataAsset）。
 *
 * 将宠物的所有可配置项集中到一处：
 *   - 生成：蓝图类 / AIController 类 / StateTree
 *   - 感知：雷达配置 / 索敌配置
 *   - 身份：PetItemID
 *   - GAS：初始技能 / 初始属性 GE / 默认标签
 *
 * 生成后通过 AMFPetBase::ApplyPetConfig() 将所有配置写入宠物 Actor。
 * 同一份 Config 可被多个 SpawnAIManager / FMFSpawnEntry 复用。
 * 多种外形相似的宠物只需共用同一个蓝图类，Config 决定其差异。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFPetConfig : public UDataAsset
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
	// 身份
	// ----------------------------------------------------------------

	/**
	 * 对应 UMFItemDatabase 中的物品 ID（如 "Item.Pet.SlimeCat"）。
	 * 写入宠物的 PetItemID，供捕获后序列化到 FMFPetInstance 使用。
	 * 留空（NAME_None）则保留蓝图 CDO 上的默认值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName PetItemID;

	// ----------------------------------------------------------------
	// GAS
	// ----------------------------------------------------------------

	/**
	 * Spawn 后追加授予的技能列表。
	 * 与角色蓝图 CDO 上的 DefaultAbilities 叠加（不替换）。
	 * 共用蓝图时，CDO 保持空列表，全部技能在此处配置。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UMFGameplayAbilityBase>> DefaultAbilities;

	/**
	 * Spawn 后应用的初始属性 GameplayEffect（Health / MoveSpeed / Attack 等）。
	 * 共用蓝图时，CDO 的 DefaultInitEffect 留空，属性初始化完全由此处驱动。
	 * 注意：与 CDO 上的 GE 是叠加应用，请勿重复配置同一属性。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultInitEffect;

	/**
	 * Spawn 后添加的 Loose GameplayTag（阵营 / 类型等固有标签）。
	 * 与 CDO 上的 DefaultOwnedTags 叠加（不替换）。
	 * 示例：MF.Team.Enemy、MF.Pet.Type.Slime。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTagContainer DefaultOwnedTags;
};

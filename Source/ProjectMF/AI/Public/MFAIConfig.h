// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MFAttributeInitData.h"
#include "MFAIConfig.generated.h"

class UMFGameplayAbilityBase;
class UGameplayEffect;
class UMFOverheadWidget;
class UMFLootTable;

/**
 * 技能释放模式：决定该技能由 AI 自主触发还是等玩家指令。
 * 仅对玩家召唤的宠物有意义（敌人/野宠的 StateTree 不挂模式门禁，总是自动）。
 */
UENUM(BlueprintType)
enum class EMFSkillReleaseMode : uint8
{
	Auto    UMETA(DisplayName = "自动"),
	Manual  UMETA(DisplayName = "手动"),
};

/**
 * 一条"授予技能 + 释放模式"配置项。授予时若 ReleaseMode==Auto，给 ability spec 打动态标签
 * MF.SkillMode.Auto（供 STCond_CanAutoUseSkill 判定）；默认 Manual。
 */
USTRUCT(BlueprintType)
struct FMFGrantedAbility
{
	GENERATED_BODY()

	/** 要授予的技能类。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UMFGameplayAbilityBase> Ability;

	/** 释放模式（默认手动）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	EMFSkillReleaseMode ReleaseMode = EMFSkillReleaseMode::Manual;
};

/**
 * UMFAIConfig — AI 角色通用配置（DataAsset）。
 *
 * 汇总所有需要在 Blueprint/编辑器 中配置的 AI 属性：
 *   - GAS：初始技能 / 初始属性 GE / 默认标签
 *   - UI：头顶血条 Widget 类 / Z 偏移
 *   - Combat：被击闪红时长
 *
 * 在 AMFAICharacter（或其 Blueprint 子类）的 Details 面板中赋值一个
 * AIConfig 资产，BeginPlay 会将其值复制到角色属性并执行后续初始化。
 * 同一份 Config 可被多种 AI Blueprint 复用。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFAIConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// GAS — 技能系统初始化
	// -----------------------------------------------------------------------

	/** BeginPlay / 配置应用时授予的初始技能列表（每项带释放模式，默认 Manual）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<FMFGrantedAbility> DefaultAbilities;

	/** 初始 Loose GameplayTag（阵营声明，无需 GE）。通常包含 MF.Team.Enemy。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTagContainer DefaultOwnedTags;

	/** 初始属性值（MaxHealth/MoveSpeed/Attack/Defense/FleeThreshold）。Health 初始 = MaxHealth。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FMFAttributeInitData InitAttributes;

	// -----------------------------------------------------------------------
	// UI — 头顶血条
	// -----------------------------------------------------------------------

	/** 头顶血条 Widget 类（必须继承 UMFOverheadWidget）。留空则不显示血条。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMFOverheadWidget> OverheadWidgetClass;

	/** 头顶血条锚点相对于胶囊中心的 Z 偏移（cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI",
		meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float OverheadWidgetZOffset = 120.f;

	// -----------------------------------------------------------------------
	// Combat — 战斗参数
	// -----------------------------------------------------------------------

	/** 被击闪红持续时间（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat",
		meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float HitFlashDuration = 0.25f;

	// -----------------------------------------------------------------------
	// Loot — 死亡掉落
	// -----------------------------------------------------------------------

	/**
	 * 死亡掉落表（空 = 不掉落）。
	 * 玩家阵营（MF.Team.Player，含召唤宠）死亡不掉落——AMFAICharacter::HandleDeath 过滤。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UMFLootTable> LootTable;
};

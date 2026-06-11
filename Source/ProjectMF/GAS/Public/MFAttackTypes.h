// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackTypes.generated.h"

class UGameplayEffect;

/**
 * Shape of the hit detection volume used by AI attack abilities.
 * Configure via UMFAttackAbilityData.
 */
UENUM(BlueprintType)
enum class EAttackShapeType : uint8
{
	Sphere  UMETA(DisplayName = "球形"),
	Sector  UMETA(DisplayName = "扇形"),
	Box     UMETA(DisplayName = "矩形 (AABB)"),
};

/**
 * Which faction of actors the attack should affect.
 * Resolved by comparing MF.Team.* GameplayTags on caster vs. candidate.
 */
UENUM(BlueprintType)
enum class EAttackTargetFilter : uint8
{
	EnemyOnly  UMETA(DisplayName = "仅敌方"),
	AllyOnly   UMETA(DisplayName = "仅友方"),
	All        UMETA(DisplayName = "全部"),
};

/**
 * 命中附加效果的类型。决定走映射表（UMFCombatEffectSettings）中哪一行 ——
 * 即施加哪个 GE、把数值写入哪些 SetByCaller 键。
 */
UENUM(BlueprintType)
enum class EMFOnHitEffectKind : uint8
{
	Stun        UMETA(DisplayName = "眩晕"),
	Freeze      UMETA(DisplayName = "冰冻"),
	Root        UMETA(DisplayName = "缠绕"),
	Blind       UMETA(DisplayName = "致盲"),
	Slow        UMETA(DisplayName = "减速"),
	Vulnerable  UMETA(DisplayName = "易伤"),
	DamageUp    UMETA(DisplayName = "增伤"),
	Burn        UMETA(DisplayName = "燃烧"),
	Heal        UMETA(DisplayName = "治疗"),
};

/**
 * 命中时按概率施加的附加效果（眩晕 / 减速 / 易伤…）。
 * 配置在攻击数据资产（UMFAttackDataBase::OnHitEffects）；命中伤害结算后逐条 roll。
 *
 * 数值全部传参：选 Kind → 由 UMFCombatEffectSettings 映射表解析出 GE 与 SetByCaller 键，
 * 再把下方 Duration / Magnitude 写入对应键。同一个 GE 资源可配不同数值。
 * （每个 Kind 对应哪些配置值，见映射表行的 DurationTag / MagnitudeTag / MagnitudeLabel。）
 */
USTRUCT(BlueprintType)
struct FMFOnHitEffect
{
	GENERATED_BODY()

	/** 效果类型（GE 与所需数值键由映射表按此解析）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit")
	EMFOnHitEffectKind Kind = EMFOnHitEffectKind::Stun;

	/** 触发概率 [0,1]：1 = 必定，0.3 = 30%。每个目标、每次命中独立判定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.f;

	/** 持续时间（秒）。瞬发治疗不用。写入映射表行的 DurationTag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit", meta = (ClampMin = "0.0",
		EditCondition = "Kind != EMFOnHitEffectKind::Heal"))
	float Duration = 2.f;

	/** 主数值（含义随 Kind：减速倍率/易伤倍率/每跳伤害/治疗量…，见映射表）。写入映射表行的 MagnitudeTag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit", meta = (
		EditCondition = "Kind == EMFOnHitEffectKind::Slow || Kind == EMFOnHitEffectKind::Vulnerable || Kind == EMFOnHitEffectKind::DamageUp || Kind == EMFOnHitEffectKind::Burn || Kind == EMFOnHitEffectKind::Heal"))
	float Magnitude = 1.f;
};

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
 * 命中时按概率施加的附加效果（眩晕 / 减速 / 易伤…）。
 * 由攻击数据资产（UMFAttackDataBase::OnHitEffects）配置，命中伤害结算后逐条 roll。
 * Duration 通过 SetByCaller(MF.Data.Duration) 写入 GE，使同一个 GE 资源支持不同时长。
 */
USTRUCT(BlueprintType)
struct FMFOnHitEffect
{
	GENERATED_BODY()

	/** 命中时施加的效果 GE（如 GE_Stun / GE_Slow）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit")
	TSubclassOf<UGameplayEffect> Effect;

	/** 触发概率 [0,1]：1 = 必定，0.3 = 30%。每个目标、每次命中独立判定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.f;

	/** 持续时间（秒），经 SetByCaller(MF.Data.Duration) 写入 GE。仅对 Duration 策略的 GE 有效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OnHit", meta = (ClampMin = "0.0"))
	float Duration = 2.f;
};

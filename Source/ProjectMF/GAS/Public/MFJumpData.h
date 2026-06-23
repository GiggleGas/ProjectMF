// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFMoveAbilityData.h"
#include "MFJumpData.generated.h"

/**
 * Data Asset：跳跃（Jump）移动技能的可配置参数。
 *
 * 前摇 / 蓄力跟随 / 后摇 / 落点区域 / 伤害 / 过滤 等共享字段继承自 UMFMoveAbilityData；
 * 本类仅补充跳跃专有的弧线与落地参数。
 *
 * 一次跳跃：前摇(Windup) → 起跳(沿抛物线弧线飞向锁定落点) → 落地(落点 AOE 结算) → 后摇。
 * 落点 = 起跳瞬间目标的位置（不追踪），目标可走位躲避。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFJumpData : public UMFMoveAbilityData
{
	GENERATED_BODY()

public:

	/**
	 * 最大跳跃距离（cm）：落点水平距离上限，超出则夹紧到该距离。
	 * 限制跳跃不会在水平方向冲出屏幕。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "1.0"))
	float MaxJumpDistance = 800.f;

	/**
	 * 最大跳跃高度（cm）：以最大距离跳跃时的抛物线峰高。
	 * 更短的跳跃按 实际距离 / 最大距离 的比例缩放峰高，永不超过此值——
	 * 既保证短跳不会有夸张高弧，也限制跳跃不会在垂直方向顶出屏幕。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
	float MaxJumpHeight = 300.f;

	/**
	 * 最小跳跃高度（cm）：弧高下限。即使水平距离≈0（原地跳，如目标就在脚下），
	 * 也至少起跳到此高度，避免"原地只播动画不动"的观感。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
	float MinJumpHeight = 100.f;

	/** 起跳到落地的时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.05"))
	float JumpDuration = 0.6f;

	/** 落地 AOE 半径（cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "1.0"))
	float ImpactRadius = 200.f;

	// 前摇 / 蓄力跟随 / 后摇 / 落点区域 + 伤害 / 过滤 / OnHitEffects
	// 全部继承自 UMFMoveAbilityData / UMFAttackDataBase。
};

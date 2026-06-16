// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFMoveAbilityData.h"
#include "MFChargeData.generated.h"

/**
 * Data Asset：冲撞（Charge）移动技能的可配置参数。
 *
 * 前摇 / 蓄力跟随 / 后摇 / 落点区域 / 伤害 / 过滤 等共享字段继承自 UMFMoveAbilityData；
 * 本类仅补充冲撞专有的冲刺位移与命中参数。
 *
 * 一次冲撞：前摇(Windup) → 冲刺(Dash，沿锁定方向直线高速位移) → 后摇(Recovery)。
 * 冲刺途中以球形 Overlap 持续检测，命中有效目标施加伤害 + OnHitEffects；每个目标只结算一次。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFChargeData : public UMFMoveAbilityData
{
	GENERATED_BODY()

public:

	/** 冲刺速度（cm/s）。位移独立于 MoveSpeed，冲刺期间接管移动。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Dash", meta = (ClampMin = "1.0"))
	float ChargeSpeed = 1200.f;

	/** 最远冲刺距离（cm）。到达即结束冲刺进入后摇。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Dash", meta = (ClampMin = "1.0"))
	float MaxDistance = 600.f;

	/** 沿途命中检测 / 撞墙检测的球半径（cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Dash", meta = (ClampMin = "1.0"))
	float ChargeRadius = 60.f;

	/**
	 * 撞到一个有效目标即停止冲刺（true）还是穿透撞飞一路所有目标（false）。
	 * 无论哪种，每个目标一次冲撞内只结算一次伤害。
	 *
	 * 注：起步瞬间就已重叠的目标不会触发停止（仍会吃伤害）——否则贴脸发起冲撞会
	 * 在第一帧立刻撞停卡在原地。只有冲刺途中"新撞上"的目标才会触发停止。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Dash")
	bool bStopOnHit = false;

	// -----------------------------------------------------------------------
	// 冲刺跟随（飞行中转向追踪，区别于基类的蓄力跟随）
	// -----------------------------------------------------------------------

	/**
	 * 冲刺期间是否继续朝目标转向（受 MaxDashTurnRate 限制的追踪）。
	 * 关闭时冲刺为锁定方向的直线。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Follow")
	bool bFollowDuringDash = false;

	/** 冲刺跟随的最大角速度（度/秒）。每帧把冲刺方向朝目标方向旋转至多该角度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Follow",
		meta = (ClampMin = "0.0", EditCondition = "bFollowDuringDash"))
	float MaxDashTurnRate = 180.f;

	// 前摇 / 蓄力跟随 / 目标要求 / 后摇 / 落点区域 + 伤害 / 过滤 / OnHitEffects
	// 全部继承自 UMFMoveAbilityData / UMFAttackDataBase。
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackDataBase.h"
#include "MFChargeData.generated.h"

class UPaperZDAnimSequence;
class UMFAreaEffectData;

/**
 * Data Asset：冲撞（Charge）移动技能的可配置参数。
 *
 * 通用字段（AttackAnim / DamageGE / DamageMultiplier / TargetFilter / OnHitEffects）
 * 继承自 UMFAttackDataBase——其中 AttackAnim 在此当作「冲刺动画」使用。
 * 本类补充冲撞专有的前摇 / 冲刺位移 / 后摇 / 落点区域参数。
 *
 * 一次冲撞流程：前摇(Windup) → 冲刺(Dash，沿锁定方向直线高速位移) → 后摇(Recovery)。
 * 冲刺途中以球形 Overlap 持续检测，命中有效目标施加伤害 + OnHitEffects；
 * 每个目标一次冲撞内只结算一次。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFChargeData : public UMFAttackDataBase
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// 前摇 / Telegraph
	// -----------------------------------------------------------------------

	/** 前摇时长（秒）：原地蓄力，给对手反应窗口。结束后开始冲刺。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Windup", meta = (ClampMin = "0.0"))
	float WindupSeconds = 0.4f;

	/** 前摇动画（可空）。为空时前摇阶段沿用 AttackAnim（冲刺动画）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Windup")
	TObjectPtr<UPaperZDAnimSequence> WindupAnim;

	// -----------------------------------------------------------------------
	// 冲刺 / Dash
	// -----------------------------------------------------------------------

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

	/**
	 * 是否要求有威胁目标才能冲撞。
	 *   true  → 无目标时取消技能；
	 *   false → 无目标时朝角色当前朝向冲。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Dash")
	bool bRequireTarget = true;

	// -----------------------------------------------------------------------
	// 跟随 / Follow（瞄准修正）
	// -----------------------------------------------------------------------

	/**
	 * 蓄力期间是否跟随目标：开启后在前摇的前 FollowDuration 秒内持续把冲刺方向更新为
	 * 朝向目标；之后锁定方向（剩余前摇成为"承诺式预警"，目标可借此走位躲避）。
	 * 关闭时方向在激活瞬间一次锁定，整段前摇都是预警。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Follow")
	bool bFollowTarget = false;

	/**
	 * 蓄力跟随时长（秒）。应小于 WindupSeconds；运行时会被夹紧到 [0, WindupSeconds]。
	 * 仅 bFollowTarget 为真时有效。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Follow",
		meta = (ClampMin = "0.0", EditCondition = "bFollowTarget"))
	float FollowDuration = 0.2f;

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

	// -----------------------------------------------------------------------
	// 后摇 / Recovery
	// -----------------------------------------------------------------------

	/** 后摇时长（秒）：冲刺结束后的收招延迟，之后 EndAbility。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Recovery", meta = (ClampMin = "0.0"))
	float RecoverySeconds = 0.3f;

	// -----------------------------------------------------------------------
	// 落点区域（可选）
	// -----------------------------------------------------------------------

	/**
	 * 冲刺结束点生成的持续区域（可空）。复用区域子系统（UMFCombatStatics::SpawnAreaEffect）。
	 * 例：冲撞终点扬尘 / 短暂减速带。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Impact")
	TObjectPtr<UMFAreaEffectData> ImpactAreaData;

	// AttackAnim（冲刺动画）/ DamageGE / DamageMultiplier / TargetFilter / OnHitEffects
	// 继承自 UMFAttackDataBase。
};

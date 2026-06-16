// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackDataBase.h"
#include "MFMoveAbilityData.generated.h"

class UPaperZDAnimSequence;
class UMFAreaEffectData;

/**
 * 所有「移动技能」（位移类）数据资产的共享基类。
 *
 * 集中放置冲撞 / 跳跃 / 后续位移变体共用的字段：前摇 telegraph、蓄力跟随、
 * 是否需要目标、后摇、落点区域。子类只补充各自的位移参数
 * （冲撞：速度/距离/半径；跳跃：弧高/时长/落地半径 等）。
 *
 * 继承链：
 *   UMFAttackDataBase
 *     └── UMFMoveAbilityData
 *           ├── UMFChargeData
 *           └── UMFJumpData
 */
UCLASS(Abstract, BlueprintType)
class PROJECTMF_API UMFMoveAbilityData : public UMFAttackDataBase
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// 前摇 / Telegraph
	// -----------------------------------------------------------------------

	/** 前摇时长（秒）：原地蓄力，给对手反应窗口。结束后开始位移。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Windup", meta = (ClampMin = "0.0"))
	float WindupSeconds = 0.4f;

	/** 前摇动画（可空）。为空时前摇阶段沿用 AttackAnim（位移动画）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Windup")
	TObjectPtr<UPaperZDAnimSequence> WindupAnim;

	// -----------------------------------------------------------------------
	// 蓄力跟随 / Follow
	// -----------------------------------------------------------------------

	/**
	 * 蓄力期间是否跟随目标：开启后在前摇的前 FollowDuration 秒内持续重新锁定目标方向/位置；
	 * 之后冻结（剩余前摇成为「承诺式预警」，目标可借此走位躲避）。
	 * 关闭时方向/落点在激活瞬间一次锁定，整段前摇都是预警。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Follow")
	bool bFollowTarget = false;

	/**
	 * 蓄力跟随时长（秒）。应小于 WindupSeconds；运行时会被夹紧到 [0, WindupSeconds]。
	 * 仅 bFollowTarget 为真时有效。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Follow",
		meta = (ClampMin = "0.0", EditCondition = "bFollowTarget"))
	float FollowDuration = 0.2f;

	// -----------------------------------------------------------------------
	// 目标 / Recovery / 落点区域
	// -----------------------------------------------------------------------

	/**
	 * 是否要求有威胁目标才能释放。
	 *   true  → 无目标时取消技能；
	 *   false → 无目标时朝角色当前朝向。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Targeting")
	bool bRequireTarget = true;

	/** 后摇时长（秒）：位移结束后的收招延迟，之后 EndAbility。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Recovery", meta = (ClampMin = "0.0"))
	float RecoverySeconds = 0.3f;

	/**
	 * 位移结束点生成的持续区域（可空）。复用区域子系统（UMFCombatStatics::SpawnAreaEffect）。
	 * 例：冲撞终点扬尘 / 跳跃落地震荡带。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Impact")
	TObjectPtr<UMFAreaEffectData> ImpactAreaData;

	// AttackAnim（位移动画）/ DamageGE / DamageMultiplier / TargetFilter / OnHitEffects
	// 继承自 UMFAttackDataBase。
};

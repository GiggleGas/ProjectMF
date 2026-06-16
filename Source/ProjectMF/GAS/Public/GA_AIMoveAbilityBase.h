// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPetGameplayAbility.h"
#include "GA_AIMoveAbilityBase.generated.h"

class UMFMoveAbilityData;
class UPaperZDAnimSequence;
class AMFAICharacter;

namespace MFMove
{
	/** 移动技能逐帧位移的固定步长（约 60Hz）。用作位置积分的 dt 与跟随/位移 tick 间隔。 */
	constexpr float TickInterval = 1.f / 60.f;
}

/**
 * 所有「移动技能」（让施法者自身位移的技能：冲撞 / 跳跃 / 后续突进、扑击等）的共享基类。
 *
 * 封装公共流程与设施，子类只实现「位移本身」：
 *   ActivateAbility（基类）
 *     → 校验 GetMoveData() / 取目标（bRequireTarget 时无目标即取消）
 *     → AddLooseGameplayTag(GetActiveStateTag())，播前摇动画，初始锁定瞄准
 *     → 蓄力跟随（前 FollowDuration 秒重新锁定，之后冻结）
 *     → [WindupSeconds] OnWindupFinished → 播位移动画 + 接管移动(MOVE_None) + BeginMovement()
 *   BeginMovement（子类）：锁定专有参数 + 启动 MotionTimer 驱动各自位移 tick
 *   位移结束时子类调 SpawnImpactArea() + StartRecovery() 收尾。
 *
 * 子类必须实现三个钩子：GetMoveData / GetActiveStateTag / BeginMovement，
 * 并在构造函数里 SetAssetTags 设自身 Ability tag。
 */
UCLASS(Abstract)
class PROJECTMF_API UGA_AIMoveAbilityBase : public UMFPetGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_AIMoveAbilityBase();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*            TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool                                 bReplicateEndAbility,
		bool                                 bWasCancelled) override;

protected:

	// -----------------------------------------------------------------------
	// 子类钩子
	// -----------------------------------------------------------------------

	/** 返回本技能的移动数据资产（子类返回各自的 UMFChargeData / UMFJumpData）。 */
	virtual UMFMoveAbilityData* GetMoveData() const
		PURE_VIRTUAL(UGA_AIMoveAbilityBase::GetMoveData, return nullptr;);

	/** 技能进行中持有的状态标签（State_Charging / State_Jumping），供 StateTree 等待结束。 */
	virtual FGameplayTag GetActiveStateTag() const
		PURE_VIRTUAL(UGA_AIMoveAbilityBase::GetActiveStateTag, return FGameplayTag(););

	/** 前摇结束、移动已接管、位移动画已播后调用：子类锁定专有参数并启动 MotionTimer。 */
	virtual void BeginMovement()
		PURE_VIRTUAL(UGA_AIMoveAbilityBase::BeginMovement, );

	// -----------------------------------------------------------------------
	// 共享设施（子类调用）
	// -----------------------------------------------------------------------

	/** Avatar 转 AMFAICharacter；非 AI 角色返回 null。 */
	AMFAICharacter* GetAICharacter() const;

	/** 从 AI 的 ThreatComponent 取当前威胁目标；无则返回 null。 */
	AActor* GetCurrentTarget() const;

	/** 返回朝向当前威胁目标的地面平面单位方向；无目标则用角色当前朝向。 */
	FVector ComputeAimDirection() const;

	/** 接管移动：停下当前移动并切到 MOVE_None，由本技能逐帧 SetActorLocation。 */
	void TakeOverMovement();
	/** 归还移动：切回 MOVE_Walking——但若仍处于眩晕/死亡则跳过（由其各自逻辑恢复）。 */
	void RestoreMovement();

	/**
	 * 对目标去重后施加伤害 + 命中附加效果（数值取 GetMoveData()）。
	 * 返回 true 表示本次释放首次命中该目标（已结算）；false 表示之前已命中过（跳过）。
	 */
	bool ApplyHitToTarget(AActor* Target);

	/** 若配了 ImpactAreaData，在 Location 生成一个以本 avatar 为来源的持续区域。 */
	void SpawnImpactArea(const FVector& Location);

	/** 位移结束统一收尾：清 MotionTimer → 归还移动 → 后摇计时 → 结束技能。 */
	void StartRecovery();

	/** 调试开关 mf.debug.move 是否开启。 */
	bool IsMoveDebugEnabled() const;

	// -----------------------------------------------------------------------
	// 共享状态（protected 供子类位移 tick 使用）
	// -----------------------------------------------------------------------

	FGameplayAbilitySpecHandle     CachedHandle;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	/** 锁定方向（蓄力跟随期更新，FollowDuration 后冻结）。冲撞用作冲刺方向。 */
	FVector AimDirection = FVector::ForwardVector;
	/** 锁定目标点（同上冻结时机）。跳跃用作落点。 */
	FVector AimTargetLocation = FVector::ZeroVector;
	/** 锁定时是否有有效目标（false 时落点/方向退回朝向 fallback）。 */
	bool bAimHasTarget = false;

	/** 子类位移 tick 计时器（冲撞 dash / 跳跃 arc 共用此槽）。 */
	FTimerHandle MotionTimer;

	/** 本次释放中已结算过的目标，确保每个目标只受一次伤害 / 控制。 */
	TSet<TWeakObjectPtr<AActor>> HitTargets;

private:

	void OnWindupFinished();
	void WindupFollowTick();
	void StopWindupFollow();
	void RefreshAim();
	void PlayMoveAnim(UPaperZDAnimSequence* Anim);
	/** 停掉本技能播的动画 override，让 AnimBP locomotion 接管（技能时长与动画时长解耦，须显式收尾）。 */
	void StopMoveAnim();
	void FinishMove();
	void ClearAllTimers();

	FTimerHandle WindupTimer;
	FTimerHandle WindupFollowTimer;
	FTimerHandle FollowStopTimer;
	FTimerHandle RecoveryTimer;
};

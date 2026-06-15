// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPetGameplayAbility.h"
#include "GA_Charge.generated.h"

class UMFChargeData;
class AMFAICharacter;

/**
 * 冲撞（Charge）——AI 战斗者的位移攻击技能。
 *
 * 与近战 / 远程攻击平级（同属 UMFPetGameplayAbility），但检测模型不同：
 * 角色自身带着碰撞球沿锁定方向高速直线位移，沿途结算，而非在固定原点做一次形状检测。
 *
 * 流程：
 *   ActivateAbility
 *     → 校验 ChargeData / 取威胁目标（bRequireTarget 时无目标即取消）
 *     → AddLooseGameplayTag(State_Charging)，播前摇动画
 *     → [WindupSeconds] BeginDash
 *           → 锁定冲刺方向（最后一次瞄准目标），接管移动(MOVE_None)，播冲刺动画
 *           → 启动冲刺 tick（每帧位移 + 撞墙检测 + 球形命中检测）
 *     → 命中 / 撞墙 / 达最远距离 → EndDash
 *           → 恢复移动、可选生成落点区域、[RecoverySeconds] 后 EndAbility
 *
 * 中断（被眩晕 / 死亡 CancelAllAbilities）：EndAbility 负责清计时器、恢复移动、移除标签。
 *
 * 配置：在 BP 子类（或直接此类）的 ChargeData 字段挂一个 UMFChargeData 资产。
 * StateTree 通过 STTask_ActivateAttack 以 AbilityTag=MF.Ability.Pet.Move.Charge、
 * ActiveStateTag=MF.GameplayState.Charging 激活并等待。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_Charge : public UMFPetGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Charge();

	/** 所有前摇 / 冲刺 / 后摇 / 伤害参数。须在 BP defaults 中赋值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Config")
	TObjectPtr<UMFChargeData> ChargeData;

	// -----------------------------------------------------------------------
	// UGameplayAbility interface
	// -----------------------------------------------------------------------

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

	/** Avatar 转 AMFAICharacter；非 AI 角色返回 null。 */
	AMFAICharacter* GetAICharacter() const;

	/** 从 AI 的 ThreatComponent 取当前威胁目标；无则返回 null。 */
	AActor* GetCurrentTarget() const;

private:

	// 阶段推进
	void BeginDash();
	void DashTick();
	void EndDash();
	void FinishCharge();

	/** 蓄力跟随：把冲刺方向更新为朝向当前目标（前摇前 FollowDuration 秒每帧调用）。 */
	void WindupFollowTick();
	/** 蓄力跟随结束：停止更新，锁定冲刺方向（剩余前摇为承诺式预警）。 */
	void StopWindupFollow();

	/** 返回朝向当前威胁目标的地面平面单位方向；无目标则用角色当前朝向。 */
	FVector ComputeAimDirection() const;

	/** 把冲刺方向朝 Desired 旋转，本帧至多 MaxStepDeg 度（冲刺跟随用）。 */
	void SteerDashDirectionToward(const FVector& Desired, float MaxStepDeg);

	/** 接管移动：停下当前移动并切到 MOVE_None，由本技能逐帧 SetActorLocation。 */
	void TakeOverMovement();
	/** 归还移动：切回 MOVE_Walking——但若仍处于眩晕/死亡则跳过（由其各自逻辑恢复）。 */
	void RestoreMovement();

	void ClearAllTimers();

	// 跨异步缓存
	FGameplayAbilitySpecHandle     CachedHandle;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	/** 冲刺锁定方向（BeginDash 时计算，之后不追踪），投影到地面平面。 */
	FVector DashDirection = FVector::ForwardVector;

	/** 已冲刺距离（cm），达到 MaxDistance 时结束。 */
	float DistanceTraveled = 0.f;

	FTimerHandle WindupTimer;
	FTimerHandle WindupFollowTimer;
	FTimerHandle FollowStopTimer;
	FTimerHandle DashTimer;
	FTimerHandle RecoveryTimer;

	/** 本次冲撞中已结算过的目标，确保每个目标只受一次伤害 / 控制。 */
	TSet<TWeakObjectPtr<AActor>> HitTargets;

	/**
	 * 冲刺起步瞬间就已重叠的目标。这些目标不触发 bStopOnHit 的撞停（仍会吃伤害），
	 * 避免贴脸发起冲撞时第一帧立刻撞停卡在原地。
	 */
	TSet<TWeakObjectPtr<AActor>> InitialOverlapActors;
};

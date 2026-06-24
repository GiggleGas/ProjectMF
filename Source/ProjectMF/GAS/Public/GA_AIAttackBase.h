// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPetGameplayAbility.h"
#include "GA_AIAttackBase.generated.h"

class UMFAttackAbilityData;
class UMFAttackDataBase;
class UPaperZDAnimSequence;
class AMFAICharacter;

/**
 * Base class for all AI melee / AOE attack abilities.
 *
 * Pipeline (each step is a BlueprintNativeEvent — override in C++ or BP):
 *   ActivateAbility
 *     → PlayAnimationOverride(AttackData->AttackAnim)
 *     → [HitDelaySeconds later] OnHitPhaseBegin
 *           → CollectTargets  (shape query)
 *           → FilterTarget    (per-candidate)
 *           → ApplyDamageToTarget
 *     → [if bMultiHit]  repeat HitCount-1 more times, every HitInterval
 *     → [if bSustained] repeat every TickInterval for SustainedDuration
 *     → OnAttackEnd → EndAbility
 *
 * Configuration:
 *   Assign AttackData (UMFAttackAbilityData) in the Blueprint CDO.
 *   All shape / timing / damage parameters — including AttackAnim — live in the
 *   Data Asset, editable at runtime.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API UGA_AIAttackBase : public UMFPetGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_AIAttackBase();

	// -----------------------------------------------------------------------
	// Blueprint Configuration
	// -----------------------------------------------------------------------

	/** All shape / timing / damage parameters (including AttackAnim). Must be assigned in BP defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Config")
	TObjectPtr<UMFAttackAbilityData> AttackData;

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

	// -----------------------------------------------------------------------
	// Overridable Pipeline Steps
	// -----------------------------------------------------------------------

	/**
	 * World-space origin of the hit detection volume.
	 * Default: actor location + DetectionOffset rotated by actor yaw.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Detection")
	FVector GetDetectionOrigin() const;
	virtual FVector GetDetectionOrigin_Implementation() const;

	/**
	 * Forward direction for directional shapes (Sector, Box).
	 * Default: actor forward vector.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Detection")
	FVector GetDetectionDirection() const;
	virtual FVector GetDetectionDirection_Implementation() const;

	/**
	 * Performs the overlap query and returns raw (unfiltered) hit Actors.
	 * Default dispatches on AttackData->ShapeType:
	 *   Sphere  — SphereOverlapActors
	 *   Sector  — SphereOverlapActors + angle filter
	 *   Box     — BoxOverlapActors (AABB; override for oriented box)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Detection")
	TArray<AActor*> CollectTargets() const;
	virtual TArray<AActor*> CollectTargets_Implementation() const;

	/**
	 * Returns true if Candidate should receive damage.
	 * Default: skips dead actors, then checks MF.Team.* tags against AttackData->TargetFilter.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Detection")
	bool FilterTarget(AActor* Candidate) const;
	virtual bool FilterTarget_Implementation(AActor* Candidate) const;

	/**
	 * Applies AttackData->DamageGE to a single validated target.
	 * Default: builds an outgoing GE spec, writes DamageMultiplier as SetByCaller
	 *          (tag MF.Attack.Data.Damage), and applies it via the source ASC.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Damage")
	void ApplyDamageToTarget(AActor* Target);
	virtual void ApplyDamageToTarget_Implementation(AActor* Target);

	/**
	 * Called when hit detection first fires (after HitDelaySeconds).
	 * Default: runs one hit round, then schedules repeat timers for multi-hit or sustained.
	 * Override to add VFX, sound, or to suppress damage conditionally.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Events")
	void OnHitPhaseBegin();
	virtual void OnHitPhaseBegin_Implementation();

	/**
	 * Called on every sustained repeat tick.
	 * Default: runs one hit round (collect → filter → apply).
	 * Override to, e.g., shrink detection radius each tick or apply stacking debuffs.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Events")
	void OnSustainedTick();
	virtual void OnSustainedTick_Implementation();

	/**
	 * Called when the full attack sequence is complete.
	 * Default: calls EndAbility.
	 * Override to play finish VFX / broadcast events before ending.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Events")
	void OnAttackEnd();
	virtual void OnAttackEnd_Implementation();

protected:

	/** Typed accessor for the owning AI character. Returns null if avatar is not AMFAICharacter. */
	UFUNCTION(BlueprintPure, Category = "Attack")
	AMFAICharacter* GetAICharacter() const;

	/** 返回 AttackData 作为攻击数据基类（供基类读冷却等共享字段）。 */
	virtual UMFAttackDataBase* GetAttackDataBase() const override;

	/** Run one full round: CollectTargets → FilterTarget → ApplyDamageToTarget. */
	void ExecuteHitRound();

private:

	void ScheduleTimers();
	void OnMultiHitTick();
	void OnSustainedTickInternal();
	void ClearTimers();

	// --- 多波次（撼地等）---

	/** 是否处于波次模式（bWaveMode 且 Waves 非空）。 */
	bool IsWaveMode() const;

	/** 当前波（波次模式且下标有效时返回，否则 nullptr）。 */
	const struct FMFAttackWave* CurrentWave() const;

	/** 本轮检测的有效半径：波次模式取当前波 Radius，否则取 AttackData->Range。 */
	float GetEffectiveRange() const;

	/** 本轮检测的有效内半径（环形）：波次模式取当前波 InnerRadius，否则 0。 */
	float GetEffectiveInnerRadius() const;

	/** 本轮伤害系数：波次模式取当前波 DamageScale，否则 1。 */
	float GetEffectiveDamageScale() const;

	/** 推进到下一波并打击；末波后收尾。 */
	void OnWaveTick();

	/** 显示/更新“下一波”落点圆预警（复用单一 TelegraphHandle）。 */
	void ShowWaveTelegraph(int32 WaveIdx);

	/** 隐藏波次预警（幂等）。 */
	void HideWaveTelegraph();

	FTimerHandle InitialHitTimer;
	FTimerHandle RepeatTimer;

	int32  HitsFired          = 0;
	int32  SustainedTicksFired = 0;

	/** 波次模式：当前波下标 + 已排到当前波的累计时间（自激活起，秒）。 */
	int32  CurrentWaveIndex   = 0;
	float  WaveTimeAccum      = 0.f;

	/**
	 * 本次释放中已施加过命中附加效果(OnHitEffects)的目标。
	 * 控制类效果（眩晕/减速等）每个目标每次释放只施加一次——伤害仍逐轮，
	 * 避免多段/持续攻击逐轮重施导致反复眩晕。每次 ActivateAbility 清空。
	 */
	TSet<TWeakObjectPtr<AActor>> OnHitEffectsAppliedTargets;

	// Cached for use inside timer callbacks
	FGameplayAbilitySpecHandle    CachedHandle;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};

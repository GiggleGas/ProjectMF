// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "GA_AIAttackBase.generated.h"

class UMFAttackAbilityData;
class UPaperZDAnimSequence;
class AMFAICharacter;

/**
 * Base class for all AI melee / AOE attack abilities.
 *
 * Pipeline (each step is a BlueprintNativeEvent — override in C++ or BP):
 *   ActivateAbility
 *     → PlayAnimationOverride(AttackAnim)
 *     → [HitDelaySeconds later] OnHitPhaseBegin
 *           → CollectTargets  (shape query)
 *           → FilterTarget    (per-candidate)
 *           → ApplyDamageToTarget
 *     → [if bMultiHit]  repeat HitCount-1 more times, every HitInterval
 *     → [if bSustained] repeat every TickInterval for SustainedDuration
 *     → OnAttackEnd → EndAbility
 *
 * Configuration:
 *   Assign AttackData (UMFAttackAbilityData) and AttackAnim in the Blueprint CDO.
 *   All shape / damage parameters live in the Data Asset — editable at runtime.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API UGA_AIAttackBase : public UMFGameplayAbilityBase
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// Blueprint Configuration
	// -----------------------------------------------------------------------

	/** All shape / timing / damage parameters. Must be assigned in BP defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Config")
	TObjectPtr<UMFAttackAbilityData> AttackData;

	/** Animation played when this ability activates. Optional — timers still fire without it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Config")
	TObjectPtr<UPaperZDAnimSequence> AttackAnim;

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
	 * Applies AttackData->DamageGameplayEffect to a single validated target.
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

	/** Run one full round: CollectTargets → FilterTarget → ApplyDamageToTarget. */
	void ExecuteHitRound();

private:

	void ScheduleTimers();
	void OnMultiHitTick();
	void OnSustainedTickInternal();
	void ClearTimers();

	FTimerHandle InitialHitTimer;
	FTimerHandle RepeatTimer;

	int32  HitsFired          = 0;
	int32  SustainedTicksFired = 0;

	// Cached for use inside timer callbacks
	FGameplayAbilitySpecHandle    CachedHandle;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};

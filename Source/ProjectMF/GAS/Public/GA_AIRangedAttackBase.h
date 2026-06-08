// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "MFAttackTypes.h"
#include "GA_AIRangedAttackBase.generated.h"

class UPaperZDAnimSequence;
class AMFAICharacter;
class UGameplayEffect;
class UMFRangedAttackDataBase;

/**
 * Abstract base class for all AI ranged attack GameplayAbilities.
 *
 * Pipeline:
 *   ActivateAbility
 *     → GetRangedData()           if null → EndAbility(cancelled)
 *     → GetCurrentTarget()        if null → EndAbility(cancelled)
 *     → PlayAnimationOverride(Data->AttackAnim)
 *     → [Data->AnimToSpawnDelay later]  OnSpawnTimerFired()
 *           → SpawnProjectile(CachedTarget)   ← subclass implements this
 *     → GA stays Running until subclass calls EndAbility
 *
 * Subclass responsibilities:
 *   1. Override GetRangedData() to return the subclass's ranged data asset
 *      (AttackAnim + AnimToSpawnDelay are read from it by the base).
 *   2. Override SpawnProjectile_Implementation to launch the actual attack.
 *   3. Call EndAbility when the attack resolves (hit, miss, or cancelled).
 *
 * Blueprint defaults must set:
 *   AbilityTags           += MF.Ability.RangedAttack
 *   ActivationOwnedTags   += MF.Character.State.RangedAttacking (or let C++ manage it)
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API UGA_AIRangedAttackBase : public UMFGameplayAbilityBase
{
	GENERATED_BODY()

public:

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
	// Overridable pipeline step — implement in C++ subclass or Blueprint
	// -----------------------------------------------------------------------

	/**
	 * Spawn the actual attack (projectile, falling rock, etc.).
	 * Called AnimToSpawnDelay seconds after ActivateAbility.
	 * Target may be null if the cached target was destroyed during the delay.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "RangedAttack")
	void SpawnProjectile(AActor* Target);
	virtual void SpawnProjectile_Implementation(AActor* Target) {}

	/**
	 * Return this ability's ranged data asset. Each subclass overrides to return its
	 * own typed asset (UMFProjectileAttackData / UMFFallingBoulderData / ...).
	 * The base ActivateAbility reads AttackAnim + AnimToSpawnDelay through this — so
	 * every ranged ability is configured by a single data-asset field, no separate
	 * AttackAnim property on the GA.
	 */
	virtual UMFRangedAttackDataBase* GetRangedData() const { return nullptr; }

protected:

	// Cached across the async gap (timer → SpawnProjectile → OnResolved).
	FGameplayAbilitySpecHandle     CachedHandle;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	TWeakObjectPtr<AActor>         CachedTarget;
	FTimerHandle                   SpawnTimer;

	// -----------------------------------------------------------------------
	// Shared helpers — call from subclass SpawnProjectile / resolved callbacks
	// -----------------------------------------------------------------------

	/** Apply DamageGE to Target using SetByCaller (MF.Attack.Data.Damage = DamageMultiplier). */
	void ApplyDamageToTarget(
		AActor*                      Target,
		TSubclassOf<UGameplayEffect> DamageGE,
		float                        DamageMultiplier);

	/** Return true if Candidate should be damaged (dead-check + team filter). */
	bool FilterTarget(AActor* Candidate, EAttackTargetFilter Filter) const;

	/** Return avatar cast to AMFAICharacter; null if avatar is not an AI character. */
	AMFAICharacter* GetAICharacter() const;

	/** Return the current threat target from the AI's ThreatComponent; null if none. */
	AActor* GetCurrentTarget() const;

private:
	void OnSpawnTimerFired();
};

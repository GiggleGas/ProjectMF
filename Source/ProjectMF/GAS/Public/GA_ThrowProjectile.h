// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AIRangedAttackBase.h"
#include "MFProjectileTypes.h"
#include "GA_ThrowProjectile.generated.h"

class UMFProjectileAttackData;
class UMFRangedAttackDataBase;
struct FMFProjectileResult;

/**
 * Ranged attack ability: launches a straight-line projectile via UMFProjectileSubsystem.
 *
 * Flow:
 *   ActivateAbility (base reads AttackAnim + AnimToSpawnDelay via GetRangedData)
 *   → [delay] SpawnProjectile → Subsystem::Launch → GA stays Running
 *   → HandleProjectileResolved callback:
 *       HitTarget  → FilterTarget → ApplyDamageToTarget → EndAbility
 *       MaxRange   → optional splash → EndAbility
 *       Cancelled  → EndAbility
 *
 * Blueprint setup:
 *   Parent class  : UGA_ThrowProjectile
 *   ProjectileData: assign a UMFProjectileAttackData asset
 *   AbilityTags   : MF.Ability.RangedAttack
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_ThrowProjectile : public UGA_AIRangedAttackBase
{
	GENERATED_BODY()

public:

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool                                 bReplicateEndAbility,
		bool                                 bWasCancelled) override;

	virtual void SpawnProjectile_Implementation(AActor* Target) override;

	virtual UMFRangedAttackDataBase* GetRangedData() const override;

protected:

	/** All projectile parameters (speed, range, mesh, damage…). Must be assigned in BP defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedAttack|Config")
	TObjectPtr<UMFProjectileAttackData> ProjectileData;

private:

	void HandleProjectileResolved(const FMFProjectileResult& Result);

	FMFProjectileHandle ActiveHandle;
};

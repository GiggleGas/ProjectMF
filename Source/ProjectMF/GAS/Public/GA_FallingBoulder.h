// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AIRangedAttackBase.h"
#include "MFProjectileTypes.h"
#include "GA_FallingBoulder.generated.h"

class UMFFallingBoulderData;

/**
 * Ranged attack ability: drops a boulder straight down onto the target's position.
 *
 * Reuses UMFProjectileSubsystem — no separate Actor needed:
 *   Origin    = TargetPos + (0, 0, FallHeight)
 *   Direction = (0, 0, -1)
 *   MaxRange  = FallHeight   ← resolved as "hit ground"
 *
 * On landing (MaxRange resolve), performs a sphere overlap at FinalPosition
 * and applies area damage to all valid targets within ImpactRadius.
 * Direct mid-air pawn hits (HitTarget resolve) also trigger area damage at
 * the collision point.
 *
 * Blueprint setup:
 *   Parent class : UGA_FallingBoulder
 *   BoulderData  : assign a UMFFallingBoulderData asset
 *   AbilityTags  : MF.Ability.RangedAttack
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_FallingBoulder : public UGA_AIRangedAttackBase
{
	GENERATED_BODY()

public:

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

	virtual void SpawnProjectile_Implementation(AActor* Target) override;

protected:

	/** All fall / impact / damage parameters. Must be assigned in BP defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedAttack|Config")
	TObjectPtr<UMFFallingBoulderData> BoulderData;

private:

	/** Handles both landing (MaxRange) and direct mid-air hit (HitTarget). */
	void HandleBoulderResolved(const FMFProjectileResult& Result);

	FMFProjectileHandle ActiveHandle;
};

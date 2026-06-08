// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AIRangedAttackBase.h"
#include "MFProjectileTypes.h"
#include "GA_BulletCurtain.generated.h"

class UMFBulletCurtainData;
class UMFRangedAttackDataBase;

/**
 * Danmaku-style ranged ability: fires multiple projectiles per burst at configured
 * angles, rotating the base angle by RotationPerBurst each burst.
 *
 * Base angle always starts at 0° (world +X) and increments each burst — no dependency
 * on the AI's facing direction.
 *
 * Does not use the single-target pipeline from GA_AIRangedAttackBase (no target lock).
 * Inherits GA_AIRangedAttackBase only for FilterTarget / ApplyDamageToTarget helpers
 * and the State_RangedAttacking tag management in EndAbility.
 *
 * Blueprint setup:
 *   Parent class      : UGA_BulletCurtain
 *   BulletCurtainData : assign a UMFBulletCurtainData asset
 *   AbilityTags       : MF.Ability.BulletCurtain
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_BulletCurtain : public UGA_AIRangedAttackBase
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

	virtual UMFRangedAttackDataBase* GetRangedData() const override;

protected:

	/** All burst / projectile / damage parameters. Must be assigned in BP defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BulletCurtain|Config")
	TObjectPtr<UMFBulletCurtainData> BulletCurtainData;

private:

	void FireBurst();
	void HandleProjectileResolved(const FMFProjectileResult& Result);

	/** Current world-space base angle (degrees). Starts at 0°, incremented per burst. */
	float CurrentBaseAngle = 0.f;

	/** How many bursts have been fired so far. */
	int32 BurstIndex = 0;

	/** Timer driving periodic bursts. */
	FTimerHandle BurstTimer;

	/** Handles for all in-flight projectiles (used for bulk cancellation in EndAbility). */
	TArray<FMFProjectileHandle> ActiveHandles;

	/** Number of launched projectiles not yet resolved. */
	int32 InFlightCount = 0;
};

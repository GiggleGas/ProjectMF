// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFRangedAttackDataBase.h"
#include "MFBulletCurtainData.generated.h"

/**
 * DataAsset that configures the bullet-curtain ranged attack (UGA_BulletCurtain).
 *
 * Each burst fires one projectile per entry in BurstAngles, all offset from
 * CurrentBaseAngle (which starts at 0° = world +X and increments by RotationPerBurst
 * after every burst).
 *
 * Common fields (AttackAnim, DamageGE, DamageMultiplier, TargetFilter, Speed,
 * MaxRange, CollisionRadius, ProjectileMesh) are inherited from
 * UMFRangedAttackDataBase. This class only adds the burst pattern.
 * (AnimToSpawnDelay is inherited but unused — bullet-curtain fires immediately.)
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFBulletCurtainData : public UMFRangedAttackDataBase
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// Burst pattern
	// -----------------------------------------------------------------------

	/** Angles (degrees) relative to the current base angle fired each burst.
	 *  Example: {0, 60, 120, 180, 240, 300} fires 6 evenly-spaced projectiles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BurstPattern")
	TArray<float> BurstAngles = { 0.f, 60.f, 120.f, 180.f, 240.f, 300.f };

	/** Degrees added to the base angle after each burst (angular velocity × interval). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BurstPattern")
	float RotationPerBurst = 15.f;

	/** Seconds between bursts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BurstPattern", meta = (ClampMin = "0.05"))
	float BurstInterval = 0.3f;

	/** Total number of bursts. Ability ends after the last burst resolves. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BurstPattern", meta = (ClampMin = "1"))
	int32 BurstCount = 10;
};

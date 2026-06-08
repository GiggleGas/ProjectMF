// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFRangedAttackDataBase.h"
#include "MFProjectileAttackData.generated.h"

/**
 * DataAsset that configures a projectile-throw ranged attack (UGA_ThrowProjectile).
 * Assign one instance to each BP child of UGA_ThrowProjectile.
 *
 * Common fields (AttackAnim, DamageGE, DamageMultiplier, TargetFilter,
 * AnimToSpawnDelay, Speed, MaxRange, CollisionRadius, ProjectileMesh) are
 * inherited from UMFRangedAttackDataBase. This class only adds max-range splash.
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFProjectileAttackData : public UMFRangedAttackDataBase
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// Max-range splash
	// -----------------------------------------------------------------------

	/** If true, the projectile deals splash damage in SplashRadius when it reaches MaxRange without hitting a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Splash")
	bool bSplashOnMaxRange = false;

	/** Splash damage radius in cm (only used when bSplashOnMaxRange is true). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Splash",
		meta = (EditCondition = "bSplashOnMaxRange", ClampMin = "1.0"))
	float SplashRadius = 80.f;
};

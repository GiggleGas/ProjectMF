// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFRangedAttackDataBase.h"
#include "MFFallingBoulderData.generated.h"

/**
 * DataAsset that configures a falling-boulder ranged attack (UGA_FallingBoulder).
 *
 * The boulder is simulated as a straight-down projectile via UMFProjectileSubsystem,
 * reusing the inherited ranged-projectile fields:
 *   Origin    = Target position + (0, 0, MaxRange)   ← MaxRange = fall height
 *   Direction = (0, 0, -1)
 *   Speed     = downward travel speed
 *   MaxRange resolved = "hit ground"
 *   ProjectileMesh = boulder mesh
 *
 * On landing, a sphere overlap at the final position triggers area damage in ImpactRadius.
 *
 * Common fields (AttackAnim, DamageGE, DamageMultiplier, TargetFilter, AnimToSpawnDelay,
 * Speed, MaxRange, CollisionRadius, ProjectileMesh) are inherited from
 * UMFRangedAttackDataBase. This class only adds the landing impact radius.
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFFallingBoulderData : public UMFRangedAttackDataBase
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// Impact
	// -----------------------------------------------------------------------

	/** Sphere overlap radius at the landing point for area damage (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact", meta = (ClampMin = "1.0"))
	float ImpactRadius = 150.f;
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFAttackTypes.h"
#include "MFFallingBoulderData.generated.h"

class UStaticMesh;
class UGameplayEffect;

/**
 * DataAsset that configures a falling-boulder ranged attack (UGA_FallingBoulder).
 *
 * The boulder is simulated as a straight-down projectile via UMFProjectileSubsystem:
 *   Origin    = Target position + (0, 0, FallHeight)
 *   Direction = (0, 0, -1)
 *   MaxRange  = FallHeight   ← MaxRange resolved = "hit ground"
 *
 * On landing, a sphere overlap at the final position triggers area damage.
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFFallingBoulderData : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Seconds between ability activation and boulder spawn (matches throw-up animation). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing")
	float AnimToSpawnDelay = 0.5f;

	// -----------------------------------------------------------------------
	// Fall physics
	// -----------------------------------------------------------------------

	/** Height above target where the boulder spawns (cm). Also used as MaxRange. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fall", meta = (ClampMin = "100.0"))
	float FallHeight = 600.f;

	/** Downward travel speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fall", meta = (ClampMin = "1.0"))
	float FallSpeed = 900.f;

	/** Mesh used to render the falling boulder via ISM. Leave null for invisible boulder. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fall")
	TObjectPtr<UStaticMesh> BoulderMesh;

	/** Sweep sphere radius while falling (cm). Keep small to avoid mid-air pawn hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fall", meta = (ClampMin = "1.0"))
	float CollisionRadius = 10.f;

	// -----------------------------------------------------------------------
	// Impact
	// -----------------------------------------------------------------------

	/** Sphere overlap radius at the landing point for area damage (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact", meta = (ClampMin = "1.0"))
	float ImpactRadius = 150.f;

	// -----------------------------------------------------------------------
	// Damage
	// -----------------------------------------------------------------------

	/** GameplayEffect applied to every valid actor inside ImpactRadius on landing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageGE;

	/** SetByCaller multiplier for MF.Attack.Data.Damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	/** Who can be damaged by the impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;
};

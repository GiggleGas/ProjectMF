// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFAttackTypes.h"
#include "MFProjectileAttackData.generated.h"

class UStaticMesh;
class UGameplayEffect;

/**
 * DataAsset that configures a projectile-throw ranged attack (UGA_ThrowProjectile).
 * Assign one instance to each BP child of UGA_ThrowProjectile.
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFProjectileAttackData : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Seconds between ability activation and the projectile spawn (should match the throw animation's release frame). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing")
	float AnimToSpawnDelay = 0.2f;

	// -----------------------------------------------------------------------
	// Projectile physics
	// -----------------------------------------------------------------------

	/** Projectile travel speed in cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Speed = 800.f;

	/** Maximum travel distance in cm before the projectile dissipates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float MaxRange = 1500.f;

	/** Radius of the sweep sphere used for hit detection (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float CollisionRadius = 15.f;

	/** Mesh used to render the projectile via ISM. Leave null for invisible projectiles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;

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

	// -----------------------------------------------------------------------
	// Damage
	// -----------------------------------------------------------------------

	/** GameplayEffect applied to each valid target on hit. Must read MF.Attack.Data.Damage as SetByCaller. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageGE;

	/** Multiplier written as SetByCaller magnitude for MF.Attack.Data.Damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	/** Who can be hit by this projectile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;
};

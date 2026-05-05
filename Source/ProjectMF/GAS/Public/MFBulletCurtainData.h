// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFAttackTypes.h"
#include "MFBulletCurtainData.generated.h"

class UStaticMesh;
class UGameplayEffect;

/**
 * DataAsset that configures the bullet-curtain ranged attack (UGA_BulletCurtain).
 *
 * Each burst fires one projectile per entry in BurstAngles, all offset from
 * CurrentBaseAngle (which starts at 0° = world +X and increments by RotationPerBurst
 * after every burst).
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFBulletCurtainData : public UDataAsset
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

	// -----------------------------------------------------------------------
	// Projectile physics
	// -----------------------------------------------------------------------

	/** Travel speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float Speed = 1200.f;

	/** Maximum travel distance before the projectile expires (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float MaxRange = 2000.f;

	/** Sweep sphere radius (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float CollisionRadius = 20.f;

	/** Mesh rendered via ISM. Leave null for invisible projectiles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	// -----------------------------------------------------------------------
	// Damage
	// -----------------------------------------------------------------------

	/** GameplayEffect applied when a projectile hits a valid target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageGE;

	/** SetByCaller multiplier for MF.Attack.Data.Damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	/** Who can be damaged by the projectiles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;
};

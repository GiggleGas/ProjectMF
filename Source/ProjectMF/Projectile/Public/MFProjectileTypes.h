// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackTypes.h"           // EAttackTargetFilter

class UStaticMesh;
class UGameplayEffect;

// ============================================================================
// Enums
// ============================================================================

/** Why a projectile stopped simulating. Passed to the GA's OnResolved callback. */
enum class EMFProjectileResolveReason : uint8
{
	HitTarget,   // 命中了通过 PassesTargetFilter 的目标
	MaxRange,    // 飞行距离达到 MaxRange，未命中目标
	Cancelled,   // GA 主动取消（EndAbility bWasCancelled=true）
};

// ============================================================================
// Result
// ============================================================================

/** Payload delivered to the GA callback when a projectile resolves. */
struct FMFProjectileResult
{
	EMFProjectileResolveReason      Reason       = EMFProjectileResolveReason::Cancelled;
	TWeakObjectPtr<AActor>          HitActor;           // 仅 HitTarget 时有效
	FVector                         FinalPosition = FVector::ZeroVector;
};

// ============================================================================
// Delegate
// ============================================================================

/** Single-cast, non-dynamic delegate fired when a projectile resolves. */
DECLARE_DELEGATE_OneParam(FOnProjectileResolved, const FMFProjectileResult&)

// ============================================================================
// Handle
// ============================================================================

/**
 * Opaque handle returned by UMFProjectileSubsystem::Launch.
 * GA holds this to cancel the projectile early (EndAbility bWasCancelled).
 * UID=0 means invalid / not yet assigned.
 */
struct FMFProjectileHandle
{
	uint32 UID = 0;

	bool IsValid()    const { return UID != 0; }
	void Invalidate()       { UID = 0; }

	bool operator==(const FMFProjectileHandle& Other) const { return UID == Other.UID; }
	bool operator!=(const FMFProjectileHandle& Other) const { return UID != Other.UID; }
};

// ============================================================================
// Launch Params  (GA → Subsystem)
// ============================================================================

/**
 * All parameters needed to start simulating one projectile.
 * The GA fills this and passes it to UMFProjectileSubsystem::Launch.
 * Direction must be pre-computed and normalized by the GA.
 */
struct FMFProjectileLaunchParams
{
	FVector                         Origin          = FVector::ZeroVector;
	FVector                         Direction       = FVector::ForwardVector; // must be normalized
	float                           Speed           = 800.f;    // cm/s
	float                           MaxRange        = 1500.f;   // cm
	float                           CollisionRadius = 15.f;     // sweep sphere radius

	UStaticMesh*                    Mesh            = nullptr;  // used for ISM slot
	TWeakObjectPtr<AActor>          Instigator;                 // ignored in filter + sweep
	TSubclassOf<UGameplayEffect>    DamageGE;
	float                           DamageMultiplier = 1.f;
	EAttackTargetFilter             TargetFilter     = EAttackTargetFilter::EnemyOnly;

	FOnProjectileResolved           OnResolved;     // bound by GA before passing to Launch
};

// ============================================================================
// Instance  (Subsystem internal state, slot-based)
// ============================================================================

/**
 * Live simulation state for one projectile inside UMFProjectileSubsystem.
 * Stored in a flat TArray; bActive=false means the slot is free.
 *
 * GC note: Mesh and DamageGE are raw/TSubclassOf pointers without UPROPERTY.
 * They are safe during normal gameplay because both originate from DataAssets
 * that are strongly referenced by the GA (which is on a live ASC).
 */
struct FMFProjectileInstance
{
	uint32                          UID              = 0;
	bool                            bActive          = false;

	FVector                         CurrentPos       = FVector::ZeroVector;
	FVector                         Direction        = FVector::ForwardVector;
	float                           Speed            = 0.f;
	float                           MaxRange         = 0.f;
	float                           DistanceTraveled = 0.f;
	float                           CollisionRadius  = 15.f;

	UStaticMesh*                    Mesh             = nullptr;
	int32                           ISMInstanceIndex = -1;      // -1 = no ISM slot

	TWeakObjectPtr<AActor>          Instigator;
	TSubclassOf<UGameplayEffect>    DamageGE;
	float                           DamageMultiplier = 1.f;
	EAttackTargetFilter             TargetFilter     = EAttackTargetFilter::EnemyOnly;

	FOnProjectileResolved           OnResolved;

	void InitFromParams(const FMFProjectileLaunchParams& Params, uint32 InUID)
	{
		UID              = InUID;
		bActive          = true;
		CurrentPos       = Params.Origin;
		Direction        = Params.Direction;
		Speed            = Params.Speed;
		MaxRange         = Params.MaxRange;
		DistanceTraveled = 0.f;
		CollisionRadius  = Params.CollisionRadius;
		Mesh             = Params.Mesh;
		ISMInstanceIndex = -1;
		Instigator       = Params.Instigator;
		DamageGE         = Params.DamageGE;
		DamageMultiplier = Params.DamageMultiplier;
		TargetFilter     = Params.TargetFilter;
		OnResolved       = Params.OnResolved;
	}

	void Reset()
	{
		UID              = 0;
		bActive          = false;
		CurrentPos       = FVector::ZeroVector;
		Direction        = FVector::ForwardVector;
		Speed            = 0.f;
		MaxRange         = 0.f;
		DistanceTraveled = 0.f;
		CollisionRadius  = 15.f;
		Mesh             = nullptr;
		ISMInstanceIndex = -1;
		Instigator       = nullptr;
		DamageGE         = nullptr;
		DamageMultiplier = 1.f;
		OnResolved.Unbind();
	}
};

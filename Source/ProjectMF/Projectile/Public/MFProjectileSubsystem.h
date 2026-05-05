// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MFProjectileTypes.h"
#include "MFProjectileSubsystem.generated.h"

class AMFProjectileRenderer;

/**
 * World-scoped subsystem that drives all active projectiles each frame.
 *
 * Design:
 *   - Slot-based FMFProjectileInstance array: bActive=false means the slot is free.
 *   - Each Tick: sphere sweep → distance check → ISM position update.
 *   - Damage is NOT applied here; the GA's OnResolved callback handles it.
 *   - Implements FTickableGameObject for an independent tick outside the Actor framework.
 *
 * Usage (from a GA):
 *   FMFProjectileHandle Handle = GetWorld()->GetSubsystem<UMFProjectileSubsystem>()->Launch(Params);
 *   // ... later if cancelled:
 *   Subsystem->Cancel(Handle);
 *
 * Debug:
 *   mf.debug.projectile 1  — prints active count and draws cyan spheres each frame.
 */
UCLASS()
class PROJECTMF_API UMFProjectileSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:

	/**
	 * Register a new projectile for simulation.
	 * Params.OnResolved must be bound before calling; the delegate fires when the
	 * projectile hits a target, reaches MaxRange, or is cancelled.
	 * Returns a handle the GA can pass to Cancel().
	 */
	FMFProjectileHandle Launch(const FMFProjectileLaunchParams& Params);

	/**
	 * Cancel an active projectile early (e.g., GA EndAbility bWasCancelled=true).
	 * Fires OnResolved with Cancelled reason. Safe to call with an invalid handle.
	 */
	void Cancel(FMFProjectileHandle Handle);

	// -----------------------------------------------------------------------
	// UWorldSubsystem
	// -----------------------------------------------------------------------

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------------
	// FTickableGameObject
	// -----------------------------------------------------------------------

	virtual void   Tick(float DeltaTime) override;
	virtual bool   IsTickable() const override;
	virtual TStatId GetStatId() const override;

private:

	/** Simulation state — slot-based flat array, never shrinks. */
	TArray<FMFProjectileInstance> Instances;

	/** Indices of inactive slots in Instances available for immediate reuse. */
	TArray<int32> FreeInstanceSlots;

	/** Scene-unique ISM renderer actor; spawned in OnWorldBeginPlay. */
	UPROPERTY()
	TObjectPtr<AMFProjectileRenderer> Renderer;

	/** Monotonically-increasing UID assigned to each newly launched projectile. */
	uint32 NextUID = 1;

	void  TickInstance(FMFProjectileInstance& Inst, float DeltaTime);

	/** Settle a projectile: release its ISM slot, reset the instance, free the slot,
	 *  then fire the OnResolved callback (which may call Launch — safe because we
	 *  never access Inst after this function returns). */
	void  ResolveInstance(FMFProjectileInstance& Inst, EMFProjectileResolveReason Reason, AActor* HitActor);

	bool  PassesTargetFilter(const FMFProjectileInstance& Inst, AActor* Candidate) const;
	int32 FindInstanceSlotByUID(uint32 UID) const;
};

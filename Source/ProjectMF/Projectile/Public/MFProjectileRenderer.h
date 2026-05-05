// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MFProjectileRenderer.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Scene-unique Actor that owns one UInstancedStaticMeshComponent per unique StaticMesh type.
 * Spawned and owned by UMFProjectileSubsystem; never manually placed in a level.
 *
 * Slot management:
 *   AcquireSlot  — returns a stable int32 index in the ISM's instance array.
 *   UpdateSlot   — called each Subsystem Tick to reposition the instance.
 *   ReleaseSlot  — hides the instance (Scale=0) and returns the index to the free pool.
 *
 * RemoveInstance is NEVER called; all indices remain stable for GC lifetime of this Actor.
 */
UCLASS(NotBlueprintable, NotPlaceable)
class PROJECTMF_API AMFProjectileRenderer : public AActor
{
	GENERATED_BODY()

public:
	AMFProjectileRenderer();

	/**
	 * Allocate a slot for Mesh at InitialTransform (world space).
	 * Reuses a hidden free slot when available; otherwise appends a new ISM instance.
	 * Returns the slot index, or -1 on failure.
	 */
	int32 AcquireSlot(UStaticMesh* Mesh, const FTransform& InitialTransform);

	/** Update the world-space transform of an active slot. Called every Subsystem Tick. */
	void  UpdateSlot(UStaticMesh* Mesh, int32 SlotIndex, const FTransform& NewTransform);

	/**
	 * Hide the instance (Scale=0,0,0) and return the slot index to the free pool.
	 * Does NOT call RemoveInstance — all other indices remain valid.
	 */
	void  ReleaseSlot(UStaticMesh* Mesh, int32 SlotIndex);

private:
	/** One ISM component per unique mesh. UPROPERTY so GC tracks the components. */
	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> ISMMap;

	/** Per-mesh pool of hidden-slot indices available for reuse. Raw key is safe:
	 *  the actual strong reference lives in ISMMap. */
	TMap<UStaticMesh*, TArray<int32>> FreeSlotMap;

	/** Find or create the ISM component for this Mesh. Never returns null for a valid Mesh. */
	UInstancedStaticMeshComponent* GetOrCreateISM(UStaticMesh* Mesh);
};

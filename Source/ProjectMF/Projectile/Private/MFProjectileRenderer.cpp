// Copyright ProjectMF. All Rights Reserved.

#include "MFProjectileRenderer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AMFProjectileRenderer::AMFProjectileRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

// ============================================================================
// Public API
// ============================================================================

int32 AMFProjectileRenderer::AcquireSlot(UStaticMesh* Mesh, const FTransform& InitialTransform)
{
	UInstancedStaticMeshComponent* ISM = GetOrCreateISM(Mesh);
	if (!ISM) return -1;

	TArray<int32>& FreeSlots = FreeSlotMap.FindOrAdd(Mesh);
	if (FreeSlots.Num() > 0)
	{
		const int32 Idx = FreeSlots.Pop(false);
		// bWorldSpace=true: InitialTransform is in world space
		ISM->UpdateInstanceTransform(Idx, InitialTransform, true, true);
		return Idx;
	}

	// AddInstance with bWorldSpace=true
	return ISM->AddInstance(InitialTransform, true);
}

void AMFProjectileRenderer::UpdateSlot(UStaticMesh* Mesh, int32 SlotIndex, const FTransform& NewTransform)
{
	if (SlotIndex < 0) return;

	UInstancedStaticMeshComponent* ISM = GetOrCreateISM(Mesh);
	if (!ISM) return;

	ISM->UpdateInstanceTransform(SlotIndex, NewTransform, true, true);
}

void AMFProjectileRenderer::ReleaseSlot(UStaticMesh* Mesh, int32 SlotIndex)
{
	if (SlotIndex < 0) return;

	UInstancedStaticMeshComponent* ISM = GetOrCreateISM(Mesh);
	if (!ISM) return;

	// Scale=0 makes the instance invisible without disturbing other indices
	static const FTransform HiddenTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
	ISM->UpdateInstanceTransform(SlotIndex, HiddenTransform, true, true);

	FreeSlotMap.FindOrAdd(Mesh).Add(SlotIndex);
}

// ============================================================================
// Private
// ============================================================================

UInstancedStaticMeshComponent* AMFProjectileRenderer::GetOrCreateISM(UStaticMesh* Mesh)
{
	if (!Mesh) return nullptr;

	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = ISMMap.Find(Mesh))
		return Found->Get();

	// Dynamic component creation: attach to root, then register
	const FName CompName = *FString::Printf(TEXT("ISM_%s"), *GetNameSafe(Mesh));
	UInstancedStaticMeshComponent* NewISM =
		NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass(), CompName);

	NewISM->SetStaticMesh(Mesh);
	NewISM->SetCastShadow(false);
	NewISM->SetupAttachment(GetRootComponent());
	NewISM->RegisterComponent();

	ISMMap.Add(Mesh, NewISM);
	return NewISM;
}

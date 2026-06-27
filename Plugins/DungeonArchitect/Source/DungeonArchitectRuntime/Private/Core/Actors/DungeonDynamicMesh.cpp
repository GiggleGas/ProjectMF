//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Actors/DungeonDynamicMesh.h"

#include "AI/NavigationSystemBase.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"

#define LOCTEXT_NAMESPACE "ADungeonVoxelChunkMesh"

UDungeonDynamicMeshComponent::UDungeonDynamicMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
}

void UDungeonDynamicMeshComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup) {
	Super::FinishPhysicsAsyncCook(bSuccess, FinishedBodySetup);
	
	FNavigationSystem::UpdateComponentData(*this);
}

ADungeonDynamicMesh::ADungeonDynamicMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DynamicMeshComponent = CreateDefaultSubobject<UDungeonDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
	DynamicMeshComponent->SetMobility(EComponentMobility::Movable);
	DynamicMeshComponent->SetGenerateOverlapEvents(false);
	DynamicMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	DynamicMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseDefault;
	DynamicMeshComponent->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));		// is this necessary?

	SetRootComponent(DynamicMeshComponent);
}


#undef LOCTEXT_NAMESPACE


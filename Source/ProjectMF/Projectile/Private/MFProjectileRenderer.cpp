// Copyright ProjectMF. All Rights Reserved.

#include "MFProjectileRenderer.h"

#include "PaperFlipbookComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

AMFProjectileRenderer::AMFProjectileRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

// ============================================================================
// Camera / billboard
// ============================================================================

void AMFProjectileRenderer::RefreshCamera()
{
	if (const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CachedCamForward = PCM->GetCameraRotation().Vector();
	}
}

FRotator AMFProjectileRenderer::GetBillboardRotation() const
{
	// sprite 局部 +Y（Paper2D 法线）指向相机；与 UMFSpriteVisualComponent::TickBillboard 一致。
	const FVector ToCam = -CachedCamForward;
	if (ToCam.IsNearlyZero()) return FRotator::ZeroRotator;
	return FRotationMatrix::MakeFromYZ(ToCam, FVector::UpVector).Rotator();
}

// ============================================================================
// Pool
// ============================================================================

int32 AMFProjectileRenderer::AllocateComponent()
{
	if (FreeSlots.Num() > 0)
	{
		return FreeSlots.Pop(EAllowShrinking::No);
	}

	// 新建一个 Flipbook 组件挂根并注册（只增不删，索引恒定）。
	UPaperFlipbookComponent* NewComp = NewObject<UPaperFlipbookComponent>(this);
	NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComp->SetCastShadow(false);
	NewComp->SetupAttachment(GetRootComponent());
	NewComp->RegisterComponent();

	return Pool.Add(NewComp);
}

int32 AMFProjectileRenderer::AcquireSlot(UPaperFlipbook* Flipbook, const FVector& WorldLocation, float Scale)
{
	if (!Flipbook) return -1;

	const int32 Idx = AllocateComponent();
	UPaperFlipbookComponent* Comp = Pool.IsValidIndex(Idx) ? Pool[Idx].Get() : nullptr;
	if (!Comp) return -1;

	Comp->SetFlipbook(Flipbook);
	Comp->SetWorldScale3D(FVector(Scale));
	Comp->SetWorldLocationAndRotation(WorldLocation, GetBillboardRotation().Quaternion());
	Comp->SetVisibility(true);
	return Idx;
}

void AMFProjectileRenderer::UpdateSlot(int32 SlotIndex, const FVector& WorldLocation)
{
	if (!Pool.IsValidIndex(SlotIndex)) return;

	if (UPaperFlipbookComponent* Comp = Pool[SlotIndex].Get())
	{
		Comp->SetWorldLocationAndRotation(WorldLocation, GetBillboardRotation().Quaternion());
	}
}

void AMFProjectileRenderer::ReleaseSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex)) return;

	if (UPaperFlipbookComponent* Comp = Pool[SlotIndex].Get())
	{
		Comp->SetVisibility(false);
		Comp->SetFlipbook(nullptr);
	}
	FreeSlots.Add(SlotIndex);
}

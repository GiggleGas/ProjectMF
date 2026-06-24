// Copyright ProjectMF. All Rights Reserved.

#include "MFTelegraphSubsystem.h"
#include "MFTelegraphSettings.h"

#include "Engine/DecalActor.h"
#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

FMFTelegraphHandle UMFTelegraphSubsystem::Show(const FMFTelegraphRequest& Request)
{
	int32 Slot = INDEX_NONE;
	if (FreeSlots.Num() > 0)
	{
		Slot = FreeSlots.Pop(EAllowShrinking::No);
	}
	else if (ADecalActor* New = AcquireDecalActor())
	{
		Slot = Pool.Add(New);
	}

	if (Slot == INDEX_NONE || !Pool.IsValidIndex(Slot) || !Pool[Slot])
	{
		return FMFTelegraphHandle();
	}

	ADecalActor* Decal = Pool[Slot];
	ApplyToDecal(Decal, Request);
	Decal->SetActorHiddenInGame(false);

	FMFTelegraphHandle Handle;
	Handle.Id = NextId++;
	HandleToSlot.Add(Handle.Id, Slot);
	return Handle;
}

void UMFTelegraphSubsystem::Update(const FMFTelegraphHandle& Handle, const FMFTelegraphRequest& Request)
{
	if (const int32* Slot = HandleToSlot.Find(Handle.Id))
	{
		if (Pool.IsValidIndex(*Slot) && Pool[*Slot])
		{
			ApplyToDecal(Pool[*Slot], Request);
		}
	}
}

void UMFTelegraphSubsystem::Hide(const FMFTelegraphHandle& Handle)
{
	if (const int32* Slot = HandleToSlot.Find(Handle.Id))
	{
		if (Pool.IsValidIndex(*Slot) && Pool[*Slot])
		{
			Pool[*Slot]->SetActorHiddenInGame(true);
		}
		FreeSlots.Add(*Slot);
		HandleToSlot.Remove(Handle.Id);
	}
}

void UMFTelegraphSubsystem::Deinitialize()
{
	for (ADecalActor* Decal : Pool)
	{
		if (Decal)
		{
			Decal->Destroy();
		}
	}
	Pool.Empty();
	FreeSlots.Empty();
	HandleToSlot.Empty();

	Super::Deinitialize();
}

ADecalActor* UMFTelegraphSubsystem::AcquireDecalActor()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADecalActor* Decal = World->SpawnActor<ADecalActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Decal) return nullptr;

	Decal->SetActorEnableCollision(false);
	Decal->SetActorHiddenInGame(true);

	// 每个 decal 一个 MID（颜色在 ApplyToDecal 时设）。
	if (UMaterialInterface* Base = ResolveBaseMaterial())
	{
		if (UDecalComponent* DC = Decal->GetDecal())
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Decal);
			DC->SetDecalMaterial(MID);
		}
	}
	return Decal;
}

void UMFTelegraphSubsystem::ApplyToDecal(ADecalActor* Decal, const FMFTelegraphRequest& Request) const
{
	if (!Decal) return;
	UDecalComponent* DC = Decal->GetDecal();
	if (!DC) return;

	// 朝下投影（Pitch -90 → 投影方向朝地面）；Rect 取 Yaw 决定朝向。
	Decal->SetActorLocationAndRotation(Request.Location, FRotator(-90.f, Request.Rotation.Yaw, 0.f));

	const UMFTelegraphSettings* Settings = GetDefault<UMFTelegraphSettings>();
	const float Depth = Settings ? Settings->ProjectionDepth : 256.f;

	// DecalSize 为半尺寸：X=投影深度，YZ=地面占地半尺寸。
	// 朝下投影(Pitch -90)时 decal 本地 Z 轴对齐 Rotation.Yaw 方向、本地 Y 轴为其垂直方向，
	// 故矩形长(BoxSize.X，沿朝向)放入 DecalSize.Z，宽(BoxSize.Y)放入 DecalSize.Y。
	FVector Size;
	if (Request.Shape == EMFTelegraphShape::Circle)
	{
		Size = FVector(Depth, Request.Radius, Request.Radius);
	}
	else
	{
		Size = FVector(Depth, Request.BoxSize.Y * 0.5f, Request.BoxSize.X * 0.5f);
	}
	DC->DecalSize = Size;
	DC->MarkRenderStateDirty();

	if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(DC->GetDecalMaterial()))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Request.Color);
		MID->SetVectorParameterValue(TEXT("DecalColor"), Request.Color);
	}
}

UMaterialInterface* UMFTelegraphSubsystem::ResolveBaseMaterial() const
{
	if (const UMFTelegraphSettings* Settings = GetDefault<UMFTelegraphSettings>())
	{
		if (UMaterialInterface* Mat = Settings->DecalMaterial.LoadSynchronous())
		{
			return Mat;
		}
	}
	// 兜底：引擎默认 Deferred Decal 材质（能渲染，非纯色；指定 DecalMaterial 后即纯色）。
	return UMaterial::GetDefaultMaterial(MD_DeferredDecal);
}

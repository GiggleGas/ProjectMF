// Copyright ProjectMF. All Rights Reserved.

#include "MFProjectileSubsystem.h"

#include "MFProjectileRenderer.h"
#include "MFGameplayTags.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Debug CVar
// ============================================================================

static TAutoConsoleVariable<int32> CVarProjectileDebug(
	TEXT("mf.debug.projectile"),
	0,
	TEXT("0=off  1=draw active projectiles as cyan spheres + print count each frame"),
	ECVF_Cheat);

// ============================================================================
// UWorldSubsystem
// ============================================================================

void UMFProjectileSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name            = TEXT("MFProjectileRenderer");
	SpawnParams.ObjectFlags     = RF_Transient;
	SpawnParams.bAllowDuringConstructionScript = false;

	Renderer = InWorld.SpawnActor<AMFProjectileRenderer>(
		AMFProjectileRenderer::StaticClass(), FTransform::Identity, SpawnParams);

	if (!Renderer)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("[ProjectileSubsystem] Failed to spawn AMFProjectileRenderer!"));
	}
}

void UMFProjectileSubsystem::Deinitialize()
{
	if (Renderer)
	{
		Renderer->Destroy();
		Renderer = nullptr;
	}
	Instances.Reset();
	FreeInstanceSlots.Reset();

	Super::Deinitialize();
}

// ============================================================================
// FTickableGameObject
// ============================================================================

bool UMFProjectileSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return !IsTemplate() && Renderer != nullptr && World && World->HasBegunPlay();
}

TStatId UMFProjectileSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMFProjectileSubsystem, STATGROUP_Tickables);
}

void UMFProjectileSubsystem::Tick(float DeltaTime)
{
	// Snapshot count: instances added by OnResolved callbacks this frame are skipped
	const int32 Count = Instances.Num();
	for (int32 i = 0; i < Count; ++i)
	{
		if (!Instances[i].bActive) continue;
		TickInstance(Instances[i], DeltaTime);
	}

	if (CVarProjectileDebug.GetValueOnGameThread() != 0)
	{
		int32 ActiveCount = 0;
		UWorld* World = GetWorld();
		for (int32 i = 0; i < Instances.Num(); ++i)
		{
			if (!Instances[i].bActive) continue;
			ActiveCount++;
			if (World)
			{
				DrawDebugSphere(World, Instances[i].CurrentPos,
					Instances[i].CollisionRadius, 8, FColor::Cyan, false, -1.f, 0, 1.f);
			}
		}
		MF_LOG(LogMFAbility, TEXT("[ProjectileSubsystem] Active: %d / %d slots"), ActiveCount, Instances.Num());
	}
}

// ============================================================================
// Public API
// ============================================================================

FMFProjectileHandle UMFProjectileSubsystem::Launch(const FMFProjectileLaunchParams& Params)
{
	// Find or create a slot
	int32 SlotIdx;
	if (FreeInstanceSlots.Num() > 0)
	{
		SlotIdx = FreeInstanceSlots.Pop(EAllowShrinking::No);
	}
	else
	{
		SlotIdx = Instances.AddDefaulted();
	}

	FMFProjectileInstance& Inst = Instances[SlotIdx];
	Inst.InitFromParams(Params, NextUID);

	// Acquire ISM slot for visual representation
	if (Renderer && Params.Mesh)
	{
		const FTransform InitTransform(FQuat::Identity, Params.Origin, FVector::OneVector);
		Inst.ISMInstanceIndex = Renderer->AcquireSlot(Params.Mesh, InitTransform);
	}

	FMFProjectileHandle Handle;
	Handle.UID = NextUID++;
	return Handle;
}

void UMFProjectileSubsystem::Cancel(FMFProjectileHandle Handle)
{
	if (!Handle.IsValid()) return;

	const int32 SlotIdx = FindInstanceSlotByUID(Handle.UID);
	if (SlotIdx == INDEX_NONE) return;

	ResolveInstance(Instances[SlotIdx], EMFProjectileResolveReason::Cancelled, nullptr);
}

// ============================================================================
// Private — per-instance tick
// ============================================================================

void UMFProjectileSubsystem::TickInstance(FMFProjectileInstance& Inst, float DeltaTime)
{
	const FVector Step   = Inst.Direction * (Inst.Speed * DeltaTime);
	const FVector NewPos = Inst.CurrentPos + Step;

	// ---- 1. Sweep trace (sphere) ----------------------------------------
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MFProjectileSweep), false);
	if (Inst.Instigator.IsValid())
		QueryParams.AddIgnoredActor(Inst.Instigator.Get());

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	const FCollisionShape Shape = FCollisionShape::MakeSphere(Inst.CollisionRadius);

	const bool bHit = GetWorld()->SweepSingleByObjectType(
		HitResult, Inst.CurrentPos, NewPos, FQuat::Identity, ObjParams, Shape, QueryParams);

	if (bHit)
	{
		AActor* Candidate = HitResult.GetActor();
		if (Candidate && PassesTargetFilter(Inst, Candidate))
		{
			ResolveInstance(Inst, EMFProjectileResolveReason::HitTarget, Candidate);
			return; // Inst is reset — never access it again
		}
	}

	// ---- 2. Max range check ---------------------------------------------
	Inst.DistanceTraveled += Step.Size();
	if (Inst.DistanceTraveled >= Inst.MaxRange)
	{
		ResolveInstance(Inst, EMFProjectileResolveReason::MaxRange, nullptr);
		return;
	}

	// ---- 3. Advance position + update ISM --------------------------------
	Inst.CurrentPos = NewPos;

	if (Renderer && Inst.ISMInstanceIndex >= 0 && Inst.Mesh)
	{
		const FRotator FacingRot = Inst.Direction.ToOrientationRotator();
		Renderer->UpdateSlot(Inst.Mesh, Inst.ISMInstanceIndex, FTransform(FacingRot, NewPos));
	}
}

// ============================================================================
// Private — resolve (settles an instance and fires the GA callback)
// ============================================================================

void UMFProjectileSubsystem::ResolveInstance(
	FMFProjectileInstance& Inst, EMFProjectileResolveReason Reason, AActor* HitActor)
{
	// Build result before touching Inst
	FMFProjectileResult Result;
	Result.Reason        = Reason;
	Result.HitActor      = HitActor;
	Result.FinalPosition = Inst.CurrentPos;

	// Release ISM slot
	if (Renderer && Inst.ISMInstanceIndex >= 0 && Inst.Mesh)
		Renderer->ReleaseSlot(Inst.Mesh, Inst.ISMInstanceIndex);

	// Move callback out so Reset() doesn't unbind it first
	FOnProjectileResolved Callback = MoveTemp(Inst.OnResolved);

	// Compute slot index via pointer arithmetic (TArray is contiguous)
	const int32 SlotIdx = static_cast<int32>(&Inst - Instances.GetData());
	Inst.Reset();
	FreeInstanceSlots.Add(SlotIdx);

	// Fire callback last — it may call Launch, causing Instances to reallocate.
	// After this line we must NOT access Inst (it may have moved in memory).
	Callback.ExecuteIfBound(Result);
}

// ============================================================================
// Private — target filter (mirrors GA_AIAttackBase logic)
// ============================================================================

bool UMFProjectileSubsystem::PassesTargetFilter(
	const FMFProjectileInstance& Inst, AActor* Candidate) const
{
	if (!Candidate) return false;

	UAbilitySystemComponent* CandidateASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);

	// Skip dead actors
	if (CandidateASC && CandidateASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead))
		return false;

	if (Inst.TargetFilter == EAttackTargetFilter::All) return true;

	UAbilitySystemComponent* InstigatorASC = nullptr;
	if (AActor* Instigator = Inst.Instigator.Get())
		InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);

	if (!InstigatorASC || !CandidateASC) return false;

	const bool bInstigatorIsPlayer = InstigatorASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player);
	const bool bCandidateIsPlayer  = CandidateASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player);
	const bool bSameTeam           = (bInstigatorIsPlayer == bCandidateIsPlayer);

	return (Inst.TargetFilter == EAttackTargetFilter::EnemyOnly) ? !bSameTeam : bSameTeam;
}

// ============================================================================
// Private — utilities
// ============================================================================

int32 UMFProjectileSubsystem::FindInstanceSlotByUID(uint32 UID) const
{
	for (int32 i = 0; i < Instances.Num(); ++i)
	{
		if (Instances[i].bActive && Instances[i].UID == UID)
			return i;
	}
	return INDEX_NONE;
}

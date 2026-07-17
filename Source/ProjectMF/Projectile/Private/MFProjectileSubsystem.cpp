// Copyright ProjectMF. All Rights Reserved.

#include "MFProjectileSubsystem.h"

#include "MFProjectileRenderer.h"
#include "MFGameplayTags.h"
#include "MFFactionStatics.h"
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
	// 每帧刷新一次相机前向，供本帧所有 slot 的 billboard 复用。
	if (Renderer) Renderer->RefreshCamera();

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

	// 取一个 Flipbook 池 slot 作视觉表现。
	if (Renderer && Params.Flipbook)
	{
		Inst.SlotIndex = Renderer->AcquireSlot(Params.Flipbook, Params.Origin, Params.VisualScale);
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
	// 弹道积分（半隐式欧拉）：先加重力再步进。GravityZ=0 → Velocity 不变 → 直线（与旧行为一致）。
	Inst.Velocity.Z -= Inst.GravityZ * DeltaTime;
	const FVector Step   = Inst.Velocity * DeltaTime;
	const FVector NewPos = Inst.CurrentPos + Step;

	// ---- 1. Sweep trace (sphere) ----------------------------------------
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MFProjectileSweep), false);
	if (Inst.Instigator.IsValid())
		QueryParams.AddIgnoredActor(Inst.Instigator.Get());

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	if (Inst.GravityZ > 0.f)
	{
		// 抛物线投掷物：额外检测地面/静物，下坠命中 → HitGround（落点生成区域）。
		// 直线攻击（GravityZ=0）不加此项，命中判定与旧行为完全一致。
		ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	}

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

		// 抛物线命中无 ASC 的静物（地面/墙）→ 落地。命中友方/中立 pawn 则穿过继续飞。
		if (Inst.GravityZ > 0.f)
		{
			UAbilitySystemComponent* CandASC = Candidate ?
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate) : nullptr;
			if (!CandASC)
			{
				Inst.CurrentPos = HitResult.ImpactPoint;   // 用精确落点作最终位置
				ResolveInstance(Inst, EMFProjectileResolveReason::HitGround, nullptr);
				return;
			}
		}
	}

	// ---- 2. Max range check ---------------------------------------------
	Inst.DistanceTraveled += Step.Size();
	if (Inst.DistanceTraveled >= Inst.MaxRange)
	{
		ResolveInstance(Inst, EMFProjectileResolveReason::MaxRange, nullptr);
		return;
	}

	// ---- 3. Advance position + update visual -----------------------------
	Inst.CurrentPos = NewPos;

	if (Renderer && Inst.SlotIndex >= 0)
	{
		Renderer->UpdateSlot(Inst.SlotIndex, NewPos);
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

	// 释放 Flipbook 池 slot
	if (Renderer && Inst.SlotIndex >= 0)
		Renderer->ReleaseSlot(Inst.SlotIndex);

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

	// Skip dead / carried (immune) actors
	if (CandidateASC && (CandidateASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead) ||
		CandidateASC->HasMatchingGameplayTag(MFGameplayTags::State_Carried)))
		return false;

	if (Inst.TargetFilter == EAttackTargetFilter::All) return true;

	UAbilitySystemComponent* InstigatorASC = nullptr;
	if (AActor* Instigator = Inst.Instigator.Get())
		InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);

	if (!InstigatorASC || !CandidateASC) return false;

	// 统一阵营判定：EnemyOnly 走 AreHostile（中立不被误伤）；AllyOnly 走 AreSameTeam。
	return (Inst.TargetFilter == EAttackTargetFilter::EnemyOnly)
		? UMFFactionStatics::AreHostile(InstigatorASC, CandidateASC)
		: UMFFactionStatics::AreSameTeam(InstigatorASC, CandidateASC);
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

// Copyright ProjectMF. All Rights Reserved.

#include "GA_AIAttackBase.h"

#include "MFAttackAbilityData.h"
#include "MFGameplayTags.h"
#include "MFCharacterBase.h"
#include "MFAICharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"

#include "PaperZDAnimationComponent.h"
#include "PaperZDAnimInstance.h"
#include "AnimSequences/PaperZDAnimSequence.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "MFLog.h"

// ============================================================================
// Debug CVar
// ============================================================================

static TAutoConsoleVariable<int32> CVarAttackDebug(
	TEXT("mf.debug.attack"),
	0,
	TEXT("0=off  1=draw AI attack detection shapes and hit targets in-world"),
	ECVF_Cheat);

// ============================================================================
// Debug draw helpers (file-local)
// ============================================================================

static void DrawDebugSector(
	const UWorld* World,
	const FVector& Origin,
	const FVector& Dir,
	float Range,
	float HalfAngleDeg,
	float Duration)
{
	// Two radial boundary lines
	const FVector LeftEdge  = Origin + Dir.RotateAngleAxis(-HalfAngleDeg, FVector::UpVector) * Range;
	const FVector RightEdge = Origin + Dir.RotateAngleAxis( HalfAngleDeg, FVector::UpVector) * Range;
	DrawDebugLine(World, Origin, LeftEdge,  FColor::Yellow, false, Duration, 0, 2.f);
	DrawDebugLine(World, Origin, RightEdge, FColor::Yellow, false, Duration, 0, 2.f);

	// Arc (20 segments)
	constexpr int32 NumSeg = 20;
	const float Step = (HalfAngleDeg * 2.f) / NumSeg;
	FVector Prev = LeftEdge;
	for (int32 i = 1; i <= NumSeg; ++i)
	{
		const FVector Cur = Origin + Dir.RotateAngleAxis(-HalfAngleDeg + Step * i, FVector::UpVector) * Range;
		DrawDebugLine(World, Prev, Cur, FColor::Yellow, false, Duration, 0, 2.f);
		Prev = Cur;
	}
}

static void DrawAttackShape(
	const UWorld* World,
	const UMFAttackAbilityData* Data,
	const FVector& Origin,
	const FVector& Dir,
	float Duration)
{
	if (!World || !Data) return;

	switch (Data->ShapeType)
	{
	case EAttackShapeType::Sphere:
		DrawDebugSphere(World, Origin, Data->Range, 16, FColor::Yellow, false, Duration, 0, 2.f);
		break;

	case EAttackShapeType::Box:
		DrawDebugBox(World, Origin, Data->BoxHalfExtent, FQuat::Identity, FColor::Yellow, false, Duration, 0, 2.f);
		break;

	case EAttackShapeType::Sector:
		DrawDebugSector(World, Origin, Dir.GetSafeNormal(), Data->Range, Data->HalfAngle, Duration);
		break;
	}
}

// ============================================================================
// UGameplayAbility interface
// ============================================================================

void UGA_AIAttackBase::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!AttackData)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("[GA_AIAttackBase] AttackData is null on %s — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle           = Handle;
	CachedActivationInfo   = ActivationInfo;
	HitsFired              = 0;
	SustainedTicksFired    = 0;

	// Tag: mark character as attacking
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MFGameplayTags::State_Attacking);
	}

	MF_LOG(LogMFAbility, TEXT("[GA_AIAttackBase] %s activated. Shape=%d HitDelay=%.2fs bMultiHit=%d bSustained=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		static_cast<int32>(AttackData->ShapeType),
		AttackData->HitDelaySeconds,
		AttackData->bMultiHit ? 1 : 0,
		AttackData->bSustained ? 1 : 0);

	// Play animation override if assigned
	if (AttackAnim)
	{
		if (AMFCharacterBase* Char = GetMFCharacter())
		{
			if (UPaperZDAnimationComponent* AnimComp =
					Char->FindComponentByClass<UPaperZDAnimationComponent>())
			{
				if (UPaperZDAnimInstance* AnimInst = AnimComp->GetAnimInstance())
				{
					AnimInst->PlayAnimationOverride(AttackAnim);
				}
			}
		}
	}

	// Schedule the initial hit detection
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InitialHitTimer,
			this,
			&UGA_AIAttackBase::OnHitPhaseBegin_Implementation,  // calls Execute_ internally
			AttackData->HitDelaySeconds,
			false);
	}
}

void UGA_AIAttackBase::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	ClearTimers();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::State_Attacking);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================================
// Default BlueprintNativeEvent implementations
// ============================================================================

FVector UGA_AIAttackBase::GetDetectionOrigin_Implementation() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !AttackData) return FVector::ZeroVector;

	// Rotate the local-space offset by the actor's yaw so "forward" always means actor forward
	const FRotator YawOnly(0.f, Avatar->GetActorRotation().Yaw, 0.f);
	return Avatar->GetActorLocation() + YawOnly.RotateVector(AttackData->DetectionOffset);
}

FVector UGA_AIAttackBase::GetDetectionDirection_Implementation() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar ? Avatar->GetActorForwardVector() : FVector::ForwardVector;
}

TArray<AActor*> UGA_AIAttackBase::CollectTargets_Implementation() const
{
	TArray<AActor*> Results;
	if (!AttackData) return Results;

	const UWorld* World = GetWorld();
	if (!World) return Results;

	const FVector  Origin    = GetDetectionOrigin();
	const FVector  Direction = GetDetectionDirection();

	// ObjectTypeQuery3 = ECC_Pawn
	const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 };
	const TArray<AActor*> Ignore = { GetAvatarActorFromActorInfo() };

	switch (AttackData->ShapeType)
	{
	case EAttackShapeType::Sphere:
		UKismetSystemLibrary::SphereOverlapActors(
			World, Origin, AttackData->Range, PawnType, nullptr, Ignore, Results);
		break;

	case EAttackShapeType::Box:
		UKismetSystemLibrary::BoxOverlapActors(
			World, Origin, AttackData->BoxHalfExtent, PawnType, nullptr, Ignore, Results);
		break;

	case EAttackShapeType::Sector:
	{
		// Sector = sphere overlap + dot-product angle filter
		TArray<AActor*> SphereHits;
		UKismetSystemLibrary::SphereOverlapActors(
			World, Origin, AttackData->Range, PawnType, nullptr, Ignore, SphereHits);

		const float CosHalf = FMath::Cos(FMath::DegreesToRadians(AttackData->HalfAngle));
		const FVector FwdNorm = Direction.GetSafeNormal();

		for (AActor* Actor : SphereHits)
		{
			const FVector ToTarget = (Actor->GetActorLocation() - Origin).GetSafeNormal();
			if (FVector::DotProduct(FwdNorm, ToTarget) >= CosHalf)
			{
				Results.Add(Actor);
			}
		}
		break;
	}
	}

	return Results;
}

bool UGA_AIAttackBase::FilterTarget_Implementation(AActor* Candidate) const
{
	if (!Candidate || !AttackData) return false;

	UAbilitySystemComponent* CandidateASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);

	// Skip dead actors
	if (CandidateASC && CandidateASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead))
	{
		return false;
	}

	if (AttackData->TargetFilter == EAttackTargetFilter::All) return true;

	// Compare team tags
	UAbilitySystemComponent* CasterASC = GetAbilitySystemComponentFromActorInfo();
	if (!CasterASC || !CandidateASC) return false;

	const bool bCasterIsPlayer    = CasterASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player);
	const bool bCandidateIsPlayer = CandidateASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player);
	const bool bSameTeam          = (bCasterIsPlayer == bCandidateIsPlayer);

	return (AttackData->TargetFilter == EAttackTargetFilter::EnemyOnly) ? !bSameTeam : bSameTeam;
}

void UGA_AIAttackBase::ApplyDamageToTarget_Implementation(AActor* Target)
{
	if (!Target || !AttackData || !AttackData->DamageGameplayEffect) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	if (!TargetASC || !SourceASC) return;

	FGameplayEffectSpecHandle Spec =
		MakeOutgoingGameplayEffectSpec(AttackData->DamageGameplayEffect, GetAbilityLevel());

	Spec.Data->SetSetByCallerMagnitude(
		MFGameplayTags::Attack_Data_Damage,
		AttackData->DamageMultiplier);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	MF_LOG(LogMFAbility, TEXT("[GA_AIAttackBase] Damage applied to %s (multiplier=%.2f)"),
		*GetNameSafe(Target), AttackData->DamageMultiplier);
}

void UGA_AIAttackBase::OnHitPhaseBegin_Implementation()
{
	const FVector Origin = GetDetectionOrigin();
	MF_LOG(LogMFAbility, TEXT("[GA_AIAttackBase] %s hit phase begin. Origin=(%.0f,%.0f,%.0f) Range=%.0f"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		Origin.X, Origin.Y, Origin.Z,
		AttackData ? AttackData->Range : 0.f);

	ExecuteHitRound();
	ScheduleTimers();
}

void UGA_AIAttackBase::OnSustainedTick_Implementation()
{
	ExecuteHitRound();
}

void UGA_AIAttackBase::OnAttackEnd_Implementation()
{
	MF_LOG(LogMFAbility, TEXT("[GA_AIAttackBase] %s attack sequence ended. TotalHits=%d SustainedTicks=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), HitsFired, SustainedTicksFired);

	EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
}

// ============================================================================
// Protected helpers
// ============================================================================

AMFAICharacter* UGA_AIAttackBase::GetAICharacter() const
{
	return Cast<AMFAICharacter>(GetAvatarActorFromActorInfo());
}

void UGA_AIAttackBase::ExecuteHitRound()
{
	const FVector Origin    = GetDetectionOrigin();
	const FVector Direction = GetDetectionDirection();
	TArray<AActor*> Raw     = CollectTargets();

	const bool bDebug         = CVarAttackDebug.GetValueOnGameThread() != 0;
	UWorld*    World          = GetWorld();
	constexpr float DebugDur  = 1.5f;

	if (bDebug && World)
	{
		DrawAttackShape(World, AttackData, Origin, Direction, DebugDur);
	}

	int32 ValidCount = 0;
	for (AActor* Actor : Raw)
	{
		if (FilterTarget(Actor))
		{
			ApplyDamageToTarget(Actor);
			ValidCount++;
			if (bDebug && World)
			{
				// 红色球 + "HIT" 文字 = 命中目标
				DrawDebugSphere(World, Actor->GetActorLocation(), 40.f, 8, FColor::Red, false, DebugDur, 0, 3.f);
				DrawDebugString(World, Actor->GetActorLocation() + FVector(0.f, 0.f, 80.f),
					FString::Printf(TEXT("HIT: %s"), *GetNameSafe(Actor)),
					nullptr, FColor::Red, DebugDur);
			}
		}
		else if (bDebug && World)
		{
			// 橙色球 = 检测到但被过滤掉
			DrawDebugSphere(World, Actor->GetActorLocation(), 40.f, 8, FColor::Orange, false, DebugDur, 0, 1.f);
		}
	}

	MF_LOG(LogMFAbility, TEXT("[GA_AIAttackBase] Hit round #%d: %d candidates, %d valid targets hit."),
		HitsFired + 1, Raw.Num(), ValidCount);

	HitsFired++;
}

// ============================================================================
// Private — timer management
// ============================================================================

void UGA_AIAttackBase::ScheduleTimers()
{
	if (!AttackData) return;
	UWorld* World = GetWorld();
	if (!World) return;

	if (AttackData->bSustained)
	{
		// Sustained: repeat every TickInterval, stop after SustainedDuration
		World->GetTimerManager().SetTimer(
			RepeatTimer,
			this,
			&UGA_AIAttackBase::OnSustainedTickInternal,
			AttackData->TickInterval,
			true);
	}
	else if (AttackData->bMultiHit && AttackData->HitCount > 1)
	{
		// Multi-hit: repeat (HitCount - 1) more times, then end
		World->GetTimerManager().SetTimer(
			RepeatTimer,
			this,
			&UGA_AIAttackBase::OnMultiHitTick,
			AttackData->HitInterval,
			true);
	}
	else
	{
		// Single hit: derive end time from animation duration when possible so the
		// ability always expires exactly when the animation finishes — no manual sync needed.
		const float EffectiveDuration = (AttackAnim && AttackAnim->GetTotalDuration() > 0.f)
			? AttackAnim->GetTotalDuration()
			: AttackData->AbilityDuration;

		const float Remaining = FMath::Max(EffectiveDuration - AttackData->HitDelaySeconds, 0.01f);

		MF_LOG(LogMFAbility,
			TEXT("[GA_AIAttackBase] Single-hit end timer: effectiveDuration=%.3fs (anim=%s, configured=%.3fs) remaining=%.3fs"),
			EffectiveDuration,
			AttackAnim ? TEXT("yes") : TEXT("no"),
			AttackData->AbilityDuration,
			Remaining);

		World->GetTimerManager().SetTimer(
			RepeatTimer,
			this,
			&UGA_AIAttackBase::OnAttackEnd_Implementation,
			Remaining,
			false);
	}
}

void UGA_AIAttackBase::OnMultiHitTick()
{
	if (!AttackData) return;

	ExecuteHitRound();

	if (HitsFired >= AttackData->HitCount)
	{
		ClearTimers();
		OnAttackEnd();
	}
}

void UGA_AIAttackBase::OnSustainedTickInternal()
{
	if (!AttackData) return;

	OnSustainedTick();
	SustainedTicksFired++;

	const int32 MaxTicks = FMath::CeilToInt(AttackData->SustainedDuration / AttackData->TickInterval);
	if (SustainedTicksFired >= MaxTicks)
	{
		ClearTimers();
		OnAttackEnd();
	}
}

void UGA_AIAttackBase::ClearTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialHitTimer);
		World->GetTimerManager().ClearTimer(RepeatTimer);
	}
}

// Copyright ProjectMF. All Rights Reserved.

#include "GA_FallingBoulder.h"

#include "MFGameplayTags.h"
#include "MFFallingBoulderData.h"
#include "MFProjectileSubsystem.h"
#include "MFLog.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

// ============================================================================
// UGameplayAbility interface
// ============================================================================

UGA_FallingBoulder::UGA_FallingBoulder()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pet_Ranged_Boulder));
}

UMFRangedAttackDataBase* UGA_FallingBoulder::GetRangedData() const
{
	return BoulderData;
}

void UGA_FallingBoulder::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	if (ActiveHandle.IsValid())
	{
		UWorld* World = GetWorld();
		if (UMFProjectileSubsystem* Subsystem = World ? World->GetSubsystem<UMFProjectileSubsystem>() : nullptr)
			Subsystem->Cancel(ActiveHandle);

		ActiveHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================================
// SpawnProjectile — launch a straight-down projectile from above the target
// ============================================================================

void UGA_FallingBoulder::SpawnProjectile_Implementation(AActor* Target)
{
	if (!Target || !BoulderData)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[GA_FallingBoulder] %s — null target or no BoulderData, cancelling."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	UMFProjectileSubsystem* Subsystem = World ? World->GetSubsystem<UMFProjectileSubsystem>() : nullptr;
	if (!Subsystem)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("[GA_FallingBoulder] UMFProjectileSubsystem not found!"));
		EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, true);
		return;
	}

	// Record the target's position at cast time — boulder falls to this fixed point.
	// MaxRange doubles as the fall height (spawn offset above the target).
	const FVector GroundPos  = Target->GetActorLocation();
	const FVector SpawnPos   = GroundPos + FVector(0.f, 0.f, BoulderData->MaxRange);

	FMFProjectileLaunchParams Params;
	Params.Origin          = SpawnPos;
	Params.Direction       = FVector::DownVector;          // straight down
	Params.Speed           = BoulderData->Speed;
	Params.MaxRange        = BoulderData->MaxRange;        // MaxRange hit = landed
	Params.CollisionRadius = BoulderData->CollisionRadius;
	Params.Mesh            = BoulderData->ProjectileMesh;
	Params.Instigator      = GetAvatarActorFromActorInfo();
	Params.DamageGE        = BoulderData->DamageGE;
	Params.DamageMultiplier = BoulderData->DamageMultiplier;
	Params.TargetFilter    = BoulderData->TargetFilter;
	Params.OnResolved.BindUObject(this, &UGA_FallingBoulder::HandleBoulderResolved);

	ActiveHandle = Subsystem->Launch(Params);

	MF_LOG(LogMFAbility,
		TEXT("[GA_FallingBoulder] %s → boulder spawned above %s  height=%.0f speed=%.0f radius=%.0f"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(Target),
		BoulderData->MaxRange,
		BoulderData->Speed,
		BoulderData->ImpactRadius);
}

// ============================================================================
// Resolve callback — area damage at landing point
// ============================================================================

void UGA_FallingBoulder::HandleBoulderResolved(const FMFProjectileResult& Result)
{
	ActiveHandle.Invalidate();

	// Cancelled: no damage
	if (Result.Reason == EMFProjectileResolveReason::Cancelled)
	{
		EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
		return;
	}

	// MaxRange = landed on the ground.
	// HitTarget = hit a pawn directly during the fall (treat same as landing).
	// In both cases apply area damage at the final position.
	TArray<AActor*> Overlaps;
	const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 };
	const TArray<AActor*> Ignore = { GetAvatarActorFromActorInfo() };

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(), Result.FinalPosition, BoulderData->ImpactRadius,
		PawnType, nullptr, Ignore, Overlaps);

	int32 HitCount = 0;
	for (AActor* Actor : Overlaps)
	{
		if (FilterTarget(Actor, BoulderData->TargetFilter))
		{
			ApplyDamageToTarget(Actor, BoulderData->DamageGE, BoulderData->DamageMultiplier);
			HitCount++;
		}
	}

	MF_LOG(LogMFAbility,
		TEXT("[GA_FallingBoulder] Landed at (%.0f, %.0f, %.0f) — %d target(s) hit in radius %.0f"),
		Result.FinalPosition.X, Result.FinalPosition.Y, Result.FinalPosition.Z,
		HitCount, BoulderData->ImpactRadius);

	// 落地点生成持续区域（若数据配了 AreaOnResolve），如地裂/燃烧区。
	SpawnResolveArea(Result.FinalPosition);

	EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
}

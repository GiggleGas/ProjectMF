// Copyright ProjectMF. All Rights Reserved.

#include "GA_ThrowProjectile.h"

#include "MFProjectileAttackData.h"
#include "MFProjectileSubsystem.h"
#include "MFAICharacter.h"
#include "MFLog.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

// ============================================================================
// UGameplayAbility interface
// ============================================================================

void UGA_ThrowProjectile::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	// Set delay from DataAsset before the base class sets the timer
	if (ProjectileData)
		AnimToSpawnDelay = ProjectileData->AnimToSpawnDelay;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_ThrowProjectile::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	// Cancel the in-flight projectile if the ability ends before it resolves
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
// SpawnProjectile
// ============================================================================

void UGA_ThrowProjectile::SpawnProjectile_Implementation(AActor* Target)
{
	if (!Target || !ProjectileData)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[GA_ThrowProjectile] %s — null target or no ProjectileData, cancelling."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	UMFProjectileSubsystem* Subsystem = World ? World->GetSubsystem<UMFProjectileSubsystem>() : nullptr;
	if (!Subsystem)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("[GA_ThrowProjectile] UMFProjectileSubsystem not found!"));
		EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, true);
		return;
	}

	AActor* Avatar     = GetAvatarActorFromActorInfo();
	const FVector Origin    = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
	const FVector Direction = (Target->GetActorLocation() - Origin).GetSafeNormal();

	FMFProjectileLaunchParams Params;
	Params.Origin          = Origin;
	Params.Direction       = Direction;
	Params.Speed           = ProjectileData->Speed;
	Params.MaxRange        = ProjectileData->MaxRange;
	Params.CollisionRadius = ProjectileData->CollisionRadius;
	Params.Mesh            = ProjectileData->ProjectileMesh;
	Params.Instigator      = Avatar;
	Params.DamageGE        = ProjectileData->DamageGE;
	Params.DamageMultiplier = ProjectileData->DamageMultiplier;
	Params.TargetFilter    = ProjectileData->TargetFilter;
	Params.OnResolved.BindUObject(this, &UGA_ThrowProjectile::HandleProjectileResolved);

	ActiveHandle = Subsystem->Launch(Params);

	MF_LOG(LogMFAbility,
		TEXT("[GA_ThrowProjectile] %s launched projectile → %s  speed=%.0f range=%.0f"),
		*GetNameSafe(Avatar), *GetNameSafe(Target),
		ProjectileData->Speed, ProjectileData->MaxRange);

	// GA stays Running — HandleProjectileResolved will call EndAbility
}

// ============================================================================
// Resolve callback
// ============================================================================

void UGA_ThrowProjectile::HandleProjectileResolved(const FMFProjectileResult& Result)
{
	ActiveHandle.Invalidate();

	switch (Result.Reason)
	{
	case EMFProjectileResolveReason::HitTarget:
		if (AActor* HitActor = Result.HitActor.Get())
		{
			if (FilterTarget(HitActor, ProjectileData->TargetFilter))
				ApplyDamageToTarget(HitActor, ProjectileData->DamageGE, ProjectileData->DamageMultiplier);
		}
		break;

	case EMFProjectileResolveReason::MaxRange:
		if (ProjectileData->bSplashOnMaxRange)
		{
			TArray<AActor*> Overlaps;
			const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 };
			const TArray<AActor*> Ignore = { GetAvatarActorFromActorInfo() };

			UKismetSystemLibrary::SphereOverlapActors(
				GetWorld(), Result.FinalPosition, ProjectileData->SplashRadius,
				PawnType, nullptr, Ignore, Overlaps);

			for (AActor* Actor : Overlaps)
			{
				if (FilterTarget(Actor, ProjectileData->TargetFilter))
					ApplyDamageToTarget(Actor, ProjectileData->DamageGE, ProjectileData->DamageMultiplier);
			}
		}
		break;

	case EMFProjectileResolveReason::Cancelled:
		break;
	}

	EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
}

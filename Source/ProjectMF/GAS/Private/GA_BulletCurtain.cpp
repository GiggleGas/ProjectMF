// Copyright ProjectMF. All Rights Reserved.

#include "GA_BulletCurtain.h"

#include "MFBulletCurtainData.h"
#include "MFGameplayTags.h"
#include "MFProjectileSubsystem.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"

// ============================================================================
// UGameplayAbility interface
// ============================================================================

void UGA_BulletCurtain::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	// Skip GA_AIRangedAttackBase::ActivateAbility — we don't need target-lock logic.
	// Call CommitAbility directly (cost / cooldown check).
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!BulletCurtainData)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[GA_BulletCurtain] %s — no BulletCurtainData assigned, cancelling."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cache for callbacks
	CachedHandle         = Handle;
	CachedActivationInfo = ActivationInfo;

	// Reuse the ranged attacking state tag
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		ASC->AddLooseGameplayTag(MFGameplayTags::State_RangedAttacking);

	CurrentBaseAngle = 0.f;
	BurstIndex       = 0;
	InFlightCount    = 0;
	ActiveHandles.Reset();

	MF_LOG(LogMFAbility,
		TEXT("[GA_BulletCurtain] %s — starting  bursts=%d  interval=%.2fs  angles=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		BulletCurtainData->BurstCount,
		BulletCurtainData->BurstInterval,
		BulletCurtainData->BurstAngles.Num());

	// Fire the first burst immediately, then start the interval timer
	FireBurst();

	if (BurstIndex < BulletCurtainData->BurstCount)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				BurstTimer,
				this,
				&UGA_BulletCurtain::FireBurst,
				BulletCurtainData->BurstInterval,
				true);
		}
	}
}

void UGA_BulletCurtain::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(BurstTimer);

	// Cancel any still-in-flight projectiles.
	// Move handles out first so the Cancelled callbacks don't re-enter EndAbility.
	TArray<FMFProjectileHandle> HandlesToCancel = MoveTemp(ActiveHandles);
	ActiveHandles.Reset();

	if (UWorld* World = GetWorld())
	{
		if (UMFProjectileSubsystem* Subsystem = World->GetSubsystem<UMFProjectileSubsystem>())
		{
			for (FMFProjectileHandle& H : HandlesToCancel)
			{
				if (H.IsValid())
					Subsystem->Cancel(H);
			}
		}
	}

	// Super handles State_RangedAttacking removal and UGameplayAbility::EndAbility
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================================
// Burst logic
// ============================================================================

void UGA_BulletCurtain::FireBurst()
{
	if (!BulletCurtainData) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UMFProjectileSubsystem* Subsystem = World ? World->GetSubsystem<UMFProjectileSubsystem>() : nullptr;
	if (!Subsystem) return;

	AActor* Instigator = GetAvatarActorFromActorInfo();
	if (!Instigator) return;

	const FVector Origin = Instigator->GetActorLocation();
	

	for (float Angle : BulletCurtainData->BurstAngles)
	{
		const float FinalDeg = CurrentBaseAngle + Angle;
		const float FinalRad = FMath::DegreesToRadians(FinalDeg);
		const FVector Direction = FVector(FMath::Cos(FinalRad), FMath::Sin(FinalRad), 0.f);

		FMFProjectileLaunchParams Params;
		Params.Origin           = Origin;
		Params.Direction        = Direction;
		Params.Speed            = BulletCurtainData->Speed;
		Params.MaxRange         = BulletCurtainData->MaxRange;
		Params.CollisionRadius  = BulletCurtainData->CollisionRadius;
		Params.Mesh             = BulletCurtainData->ProjectileMesh;
		Params.Instigator       = Instigator;
		Params.DamageGE         = BulletCurtainData->DamageGE;
		Params.DamageMultiplier = BulletCurtainData->DamageMultiplier;
		Params.TargetFilter     = BulletCurtainData->TargetFilter;
		Params.OnResolved.BindUObject(this, &UGA_BulletCurtain::HandleProjectileResolved);

		FMFProjectileHandle Handle = Subsystem->Launch(Params);
		if (Handle.IsValid())
		{
			ActiveHandles.Add(Handle);
			InFlightCount++;
		}
	}

	MF_LOG(LogMFAbility,
		TEXT("[GA_BulletCurtain] Burst %d/%d  baseAngle=%.1f°  projectiles=%d"),
		BurstIndex + 1, BulletCurtainData->BurstCount,
		CurrentBaseAngle,
		BulletCurtainData->BurstAngles.Num());

	CurrentBaseAngle += BulletCurtainData->RotationPerBurst;
	BurstIndex++;

	if (BurstIndex >= BulletCurtainData->BurstCount)
	{
		World->GetTimerManager().ClearTimer(BurstTimer);
		// Ability ends naturally once all in-flight projectiles resolve
	}
}

// ============================================================================
// Projectile resolve callback
// ============================================================================

void UGA_BulletCurtain::HandleProjectileResolved(const FMFProjectileResult& Result)
{
	// Cancelled fires when EndAbility is cleaning up — ignore to avoid re-entrancy
	if (Result.Reason == EMFProjectileResolveReason::Cancelled)
		return;

	InFlightCount = FMath::Max(0, InFlightCount - 1);

	if (Result.Reason == EMFProjectileResolveReason::HitTarget && Result.HitActor.IsValid())
	{
		if (FilterTarget(Result.HitActor.Get(), BulletCurtainData->TargetFilter))
			ApplyDamageToTarget(Result.HitActor.Get(), BulletCurtainData->DamageGE, BulletCurtainData->DamageMultiplier);
	}

	// End once all bursts have fired AND every in-flight projectile has resolved
	if (BurstIndex >= BulletCurtainData->BurstCount && InFlightCount <= 0)
	{
		EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
	}
}

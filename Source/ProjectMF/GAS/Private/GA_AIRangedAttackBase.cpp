// Copyright ProjectMF. All Rights Reserved.

#include "GA_AIRangedAttackBase.h"

#include "MFRangedAttackDataBase.h"
#include "MFGameplayTags.h"
#include "MFFactionStatics.h"
#include "MFAICharacter.h"
#include "MFCharacterBase.h"
#include "MFThreatComponent.h"
#include "MFCombatAttributeSet.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"

#include "PaperZDAnimationComponent.h"
#include "PaperZDAnimInstance.h"
#include "AnimSequences/PaperZDAnimSequence.h"

#include "Engine/World.h"

// ============================================================================
// UGameplayAbility interface
// ============================================================================

void UGA_AIRangedAttackBase::ActivateAbility(
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

	// Require a configured data asset (carries AttackAnim + AnimToSpawnDelay)
	UMFRangedAttackDataBase* Data = GetRangedData();
	if (!Data)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[GA_AIRangedAttackBase] %s has no ranged data asset assigned — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Require a valid target
	CachedTarget = GetCurrentTarget();
	if (!CachedTarget.IsValid())
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[GA_AIRangedAttackBase] %s has no threat target — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle         = Handle;
	CachedActivationInfo = ActivationInfo;

	// Mark AI as ranged-attacking so StateTree / animations can react
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		ASC->AddLooseGameplayTag(MFGameplayTags::State_RangedAttacking);

	MF_LOG(LogMFAbility,
		TEXT("[GA_AIRangedAttackBase] %s → target=%s  delay=%.2fs"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(CachedTarget.Get()),
		Data->AnimToSpawnDelay);

	// Play attack animation if assigned
	if (Data->AttackAnim)
	{
		if (AMFCharacterBase* Char = GetMFCharacter())
		{
			if (UPaperZDAnimationComponent* AnimComp =
					Char->FindComponentByClass<UPaperZDAnimationComponent>())
			{
				if (UPaperZDAnimInstance* AnimInst = AnimComp->GetAnimInstance())
					AnimInst->PlayAnimationOverride(Data->AttackAnim);
			}
		}
	}

	// Delay before spawning the projectile / boulder
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SpawnTimer,
			this,
			&UGA_AIRangedAttackBase::OnSpawnTimerFired,
			FMath::Max(Data->AnimToSpawnDelay, 0.01f),
			false);
	}
	// GA stays Running — subclass EndAbility when attack resolves
}

void UGA_AIRangedAttackBase::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(SpawnTimer);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		ASC->RemoveLooseGameplayTag(MFGameplayTags::State_RangedAttacking);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================================
// Protected helpers
// ============================================================================

void UGA_AIRangedAttackBase::ApplyDamageToTarget(
	AActor*                      Target,
	TSubclassOf<UGameplayEffect> DamageGE,
	float                        DamageMultiplier)
{
	if (!Target || !DamageGE) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC) return;

	float AttackValue  = 0.f;
	float OutgoingMult = 1.f;
	if (const UMFCombatAttributeSet* CombatSet = SourceASC->GetSet<UMFCombatAttributeSet>())
	{
		AttackValue  = CombatSet->GetAttack();
		OutgoingMult = CombatSet->GetOutgoingDamageMultiplier();
	}

	const float FinalMagnitude = AttackValue * DamageMultiplier * OutgoingMult;

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageGE, GetAbilityLevel());
	Spec.Data->SetSetByCallerMagnitude(MFGameplayTags::Attack_Data_Damage, FinalMagnitude);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	MF_LOG(LogMFAbility,
		TEXT("[GA_AIRangedAttackBase] Damage → %s (attack=%.1f x mult=%.2f x out=%.2f = %.1f)"),
		*GetNameSafe(Target), AttackValue, DamageMultiplier, OutgoingMult, FinalMagnitude);

	// 命中附加效果（眩晕 / 减速等，按概率）；落石 AOE 时每个目标独立 roll
	if (const UMFRangedAttackDataBase* Data = GetRangedData())
	{
		ApplyOnHitEffects(Target, Data->OnHitEffects);
	}
}

bool UGA_AIRangedAttackBase::FilterTarget(AActor* Candidate, EAttackTargetFilter Filter) const
{
	if (!Candidate) return false;

	UAbilitySystemComponent* CandidateASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);

	if (CandidateASC && CandidateASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead))
		return false;

	if (Filter == EAttackTargetFilter::All) return true;

	// 阵营判定：共享任意 MF.Team.* 标签即同队；中立与所有人都不同队。
	UAbilitySystemComponent* CasterASC = GetAbilitySystemComponentFromActorInfo();
	if (!CasterASC || !CandidateASC) return false;

	const bool bSameTeam = UMFFactionStatics::AreSameTeam(CasterASC, CandidateASC);

	return (Filter == EAttackTargetFilter::EnemyOnly) ? !bSameTeam : bSameTeam;
}

AMFAICharacter* UGA_AIRangedAttackBase::GetAICharacter() const
{
	return Cast<AMFAICharacter>(GetAvatarActorFromActorInfo());
}

AActor* UGA_AIRangedAttackBase::GetCurrentTarget() const
{
	AMFAICharacter* AI = GetAICharacter();
	if (!AI) return nullptr;

	UMFThreatComponent* ThreatComp = AI->FindComponentByClass<UMFThreatComponent>();
	return ThreatComp ? ThreatComp->GetCurrentTarget() : nullptr;
}

// ============================================================================
// Private
// ============================================================================

void UGA_AIRangedAttackBase::OnSpawnTimerFired()
{
	// Calling SpawnProjectile() (not _Implementation) routes through the UFunction dispatch,
	// so Blueprint overrides are respected — equivalent to Execute_ but user-accessible.
	SpawnProjectile(CachedTarget.Get());
}

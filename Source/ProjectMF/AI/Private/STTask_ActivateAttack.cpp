// Copyright ProjectMF. All Rights Reserved.

#include "STTask_ActivateAttack.h"

#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

// ============================================================================
// Link
// ============================================================================

bool FSTTask_ActivateAttack::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

// ============================================================================
// EnterState
// ============================================================================

EStateTreeRunStatus FSTTask_ActivateAttack::EnterState(
	FStateTreeExecutionContext&        Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	UAbilitySystemComponent* ASC = GetASC(Context);
	if (!ASC)
	{
		MF_LOG_ERROR(LogMFAbility,
			TEXT("[STTask_ActivateAttack] Cannot get ASC from %s — pawn is not AMFCharacterBase."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	if (!InstanceData.AbilityTag.IsValid() || !InstanceData.ActiveStateTag.IsValid())
	{
		MF_LOG_ERROR(LogMFAbility,
			TEXT("[STTask_ActivateAttack] %s — AbilityTag or ActiveStateTag is not set in StateTree editor!"),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	// 查找 AbilityTag 对应的已授予技能
	FGameplayAbilitySpec* AttackSpec = nullptr;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(InstanceData.AbilityTag))
		{
			AttackSpec = &Spec;
			break;
		}
	}

	if (!AttackSpec)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[STTask_ActivateAttack] %s has no granted ability with tag %s — "
			     "add it to DefaultAbilities and set AbilityTags on the ability BP."),
			*GetNameSafe(Controller.GetPawn()),
			*InstanceData.AbilityTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ActiveSpecHandle = AttackSpec->Handle;

	MF_LOG(LogMFAbility, TEXT("[STTask_ActivateAttack] %s → activating %s (tag=%s)"),
		*GetNameSafe(Controller.GetPawn()),
		*GetNameSafe(AttackSpec->Ability),
		*InstanceData.AbilityTag.ToString());

	const bool bActivated = ASC->TryActivateAbility(InstanceData.ActiveSpecHandle);
	if (!bActivated)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[STTask_ActivateAttack] TryActivateAbility failed on %s — "
			     "check cooldown / blocking tags / CanActivate."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	MF_LOG(LogMFAbility, TEXT("[STTask_ActivateAttack] Ability activated on %s — waiting for %s to clear."),
		*GetNameSafe(Controller.GetPawn()),
		*InstanceData.ActiveStateTag.ToString());

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Tick
// ============================================================================

EStateTreeRunStatus FSTTask_ActivateAttack::Tick(
	FStateTreeExecutionContext& Context,
	const float                 DeltaTime) const
{
	UAbilitySystemComponent* ASC = GetASC(Context);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	// ActiveStateTag 消失表示技能已正常结束
	if (!ASC->HasMatchingGameplayTag(InstanceData.ActiveStateTag))
	{
		const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
		MF_LOG(LogMFAbility, TEXT("[STTask_ActivateAttack] Ability finished naturally on %s → Succeeded."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// ExitState
// ============================================================================

void FSTTask_ActivateAttack::ExitState(
	FStateTreeExecutionContext&        Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	UAbilitySystemComponent* ASC    = GetASC(Context);

	if (!ASC || !InstanceData.ActiveSpecHandle.IsValid()) return;

	// 只在技能仍激活时取消（正常结束时 tag 已移除，无需重复取消）
	if (ASC->HasMatchingGameplayTag(InstanceData.ActiveStateTag))
	{
		ASC->CancelAbilityHandle(InstanceData.ActiveSpecHandle);

		const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[STTask_ActivateAttack] Ability interrupted on %s by state transition — cancelled."),
			*GetNameSafe(Controller.GetPawn()));
	}

	InstanceData.ActiveSpecHandle = FGameplayAbilitySpecHandle();
}

// ============================================================================
// Private helper
// ============================================================================

UAbilitySystemComponent* FSTTask_ActivateAttack::GetASC(FStateTreeExecutionContext& Context) const
{
	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	APawn* Pawn = Controller.GetPawn();
	AMFCharacterBase* MFChar = Cast<AMFCharacterBase>(Pawn);
	return MFChar ? MFChar->GetAbilitySystemComponent() : nullptr;
}

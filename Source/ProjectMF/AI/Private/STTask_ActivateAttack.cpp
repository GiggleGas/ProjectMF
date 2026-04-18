// Copyright ProjectMF. All Rights Reserved.

#include "STTask_ActivateAttack.h"

#include "MFGameplayTags.h"
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

	// 查找带 MF.Ability.Attack tag 的已授予技能（配置在 AI 蓝图的 AbilityTags 上）
	FGameplayAbilitySpec* AttackSpec = nullptr;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(MFGameplayTags::Ability_Attack))
		{
			AttackSpec = &Spec;
			break;
		}
	}

	if (!AttackSpec)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[STTask_ActivateAttack] %s has no granted ability with tag MF.Ability.Attack — "
			     "add it to DefaultAbilities and set AbilityTags on the attack ability BP."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ActiveSpecHandle = AttackSpec->Handle;

	MF_LOG(LogMFAbility, TEXT("[STTask_ActivateAttack] %s → activating %s"),
		*GetNameSafe(Controller.GetPawn()), *GetNameSafe(AttackSpec->Ability));

	const bool bActivated = ASC->TryActivateAbility(InstanceData.ActiveSpecHandle);
	if (!bActivated)
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[STTask_ActivateAttack] TryActivateAbility failed on %s — "
			     "check cooldown / blocking tags / CanActivate."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	MF_LOG(LogMFAbility, TEXT("[STTask_ActivateAttack] Attack activated on %s — waiting for completion."),
		*GetNameSafe(Controller.GetPawn()));

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

	// GA_AIAttackBase 在激活时添加 State_Attacking tag，结束时移除
	if (!ASC->HasMatchingGameplayTag(MFGameplayTags::State_Attacking))
	{
		const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
		MF_LOG(LogMFAbility, TEXT("[STTask_ActivateAttack] Attack finished naturally on %s → Succeeded."),
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
	if (ASC->HasMatchingGameplayTag(MFGameplayTags::State_Attacking))
	{
		ASC->CancelAbilityHandle(InstanceData.ActiveSpecHandle);

		const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
		MF_LOG_WARNING(LogMFAbility,
			TEXT("[STTask_ActivateAttack] Attack interrupted on %s by state transition — ability cancelled."),
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

// Copyright ProjectMF. All Rights Reserved.

#include "STTask_ExecuteCommandSkill.h"

#include "MFPetCommandComponent.h"
#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTTask_ExecuteCommandSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_ExecuteCommandSkill::EnterState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	APawn* Pawn = Controller.GetPawn();

	UMFPetCommandComponent* CommandComp = Pawn ? Pawn->FindComponentByClass<UMFPetCommandComponent>() : nullptr;
	if (!CommandComp || !CommandComp->HasCommand())
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[ExecuteCommandSkill] %s has no pending command — Failed."),
			*GetNameSafe(Pawn));
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayTag SkillTag = CommandComp->GetCommand().SkillTag;
	// 已取到技能标签，立即消费指令（执行中到来的新指令可干净抢占）。
	CommandComp->ConsumeCommand();

	UAbilitySystemComponent* ASC = GetASC(Context);
	if (!ASC || !SkillTag.IsValid())
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[ExecuteCommandSkill] %s — null ASC or invalid SkillTag (%s) — Failed."),
			*GetNameSafe(Pawn), *SkillTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	// 找 SkillTag 对应的已授予技能。
	FGameplayAbilitySpec* SkillSpec = nullptr;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(SkillTag))
		{
			SkillSpec = &Spec;
			break;
		}
	}

	if (!SkillSpec)
	{
		MF_LOG_WARNING(LogMFAI,
			TEXT("[ExecuteCommandSkill] %s has no granted ability with tag %s — Failed."),
			*GetNameSafe(Pawn), *SkillTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ActiveSpecHandle = SkillSpec->Handle;

	if (!ASC->TryActivateAbility(InstanceData.ActiveSpecHandle))
	{
		MF_LOG_WARNING(LogMFAI,
			TEXT("[ExecuteCommandSkill] TryActivateAbility(%s) failed on %s — check cooldown / blocking tags."),
			*SkillTag.ToString(), *GetNameSafe(Pawn));
		InstanceData.ActiveSpecHandle = FGameplayAbilitySpecHandle();
		return EStateTreeRunStatus::Failed;
	}

	MF_LOG(LogMFAI, TEXT("[ExecuteCommandSkill] %s → activated skill %s, waiting for it to finish."),
		*GetNameSafe(Pawn), *SkillTag.ToString());

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_ExecuteCommandSkill::Tick(
	FStateTreeExecutionContext& Context,
	const float                 DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	UAbilitySystemComponent* ASC = GetASC(Context);
	if (!ASC || !InstanceData.ActiveSpecHandle.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 技能 spec 不再 active 表示已结束（通用于任何技能，无需 per-skill 状态标签）。
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.ActiveSpecHandle);
	if (!Spec || !Spec->IsActive())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_ExecuteCommandSkill::ExitState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	UAbilitySystemComponent* ASC    = GetASC(Context);

	if (!ASC || !InstanceData.ActiveSpecHandle.IsValid())
	{
		return;
	}

	// 被转换打断（如新指令抢占）时，技能仍在跑则取消。
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.ActiveSpecHandle);
	if (Spec && Spec->IsActive())
	{
		ASC->CancelAbilityHandle(InstanceData.ActiveSpecHandle);
	}

	InstanceData.ActiveSpecHandle = FGameplayAbilitySpecHandle();
}

UAbilitySystemComponent* FSTTask_ExecuteCommandSkill::GetASC(FStateTreeExecutionContext& Context) const
{
	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	AMFCharacterBase* MFChar = Cast<AMFCharacterBase>(Controller.GetPawn());
	return MFChar ? MFChar->GetAbilitySystemComponent() : nullptr;
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFPetCommandComponent.h"

#include "MFPetAIController.h"
#include "MFGameplayTags.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"

UMFPetCommandComponent::UMFPetCommandComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMFPetCommandComponent::IssueCommand(const FMFPetCommand& Command)
{
	PendingCommand = Command;
	bHasCommand    = true;

	// 向本宠 StateTree 发打断事件（M0：发出即可，M1 给 StateTree 加顶层转换后才被消费）。
	APawn* Pawn = Cast<APawn>(GetOwner());
	AMFPetAIController* Controller = Pawn ? Cast<AMFPetAIController>(Pawn->GetController()) : nullptr;
	if (Controller)
	{
		Controller->SendStateTreeEvent(MFGameplayTags::Event_PlayerCommand);
	}
	else
	{
		MF_LOG_WARNING(LogMFAI,
			TEXT("[PetCommand] %s issued a command but has no AMFPetAIController — stored only, no StateTree event."),
			*GetNameSafe(GetOwner()));
	}

	MF_LOG(LogMFAI, TEXT("[PetCommand] %s <- command Type=%d Loc=(%.0f,%.0f,%.0f) Skill=%s"),
		*GetNameSafe(GetOwner()), static_cast<int32>(Command.Type),
		Command.TargetLocation.X, Command.TargetLocation.Y, Command.TargetLocation.Z,
		*Command.SkillTag.ToString());
}

void UMFPetCommandComponent::ConsumeCommand()
{
	bHasCommand        = false;
	PendingCommand.Type = EMFCommandType::None;
}

FGameplayTag UMFPetCommandComponent::GetReadyManualSkillTag() const
{
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return FGameplayTag();
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability)
		{
			continue;
		}

		// 手动 = spec 无 MF.SkillMode.Auto 标签。
		if (Spec.GetDynamicSpecSourceTags().HasTag(MFGameplayTags::SkillMode_Auto))
		{
			continue;
		}

		// 跳过冷却中的（冷却身份标签 = 技能自身 AbilityTag）。
		if (const FGameplayTagContainer* CooldownTags = Spec.Ability->GetCooldownTags())
		{
			if (ASC->HasAnyMatchingGameplayTags(*CooldownTags))
			{
				continue;
			}
		}

		// 返回该技能的 AbilityTag（每个 GA 仅设一个 MF.Ability.* 资产标签）。
		const FGameplayTagContainer& AssetTags = Spec.Ability->GetAssetTags();
		if (!AssetTags.IsEmpty())
		{
			return AssetTags.First();
		}
	}

	return FGameplayTag();
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFPetCommandComponent.h"

#include "MFPetAIController.h"
#include "MFGameplayTags.h"
#include "MFLog.h"

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

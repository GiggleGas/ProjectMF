// Copyright ProjectMF. All Rights Reserved.

#include "STTask_ReadCommandLocation.h"

#include "MFPetCommandComponent.h"
#include "MFLog.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTTask_ReadCommandLocation::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_ReadCommandLocation::EnterState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	APawn* Pawn = Controller.GetPawn();
	UMFPetCommandComponent* CommandComp = Pawn ? Pawn->FindComponentByClass<UMFPetCommandComponent>() : nullptr;
	if (!CommandComp || !CommandComp->HasCommand())
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[ReadCommandLocation] %s has no pending command — Failed."),
			*GetNameSafe(Pawn));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ResultLocation = CommandComp->GetCommand().TargetLocation;

	// 已取到落点，立即消费指令（执行中到来的新指令可干净抢占）。
	CommandComp->ConsumeCommand();

	MF_LOG(LogMFAI, TEXT("[ReadCommandLocation] %s → move target=(%.0f,%.0f,%.0f)"),
		*GetNameSafe(Pawn),
		InstanceData.ResultLocation.X, InstanceData.ResultLocation.Y, InstanceData.ResultLocation.Z);

	return EStateTreeRunStatus::Succeeded;
}

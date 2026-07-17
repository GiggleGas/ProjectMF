// Copyright ProjectMF. All Rights Reserved.

#include "STTask_GetHomeLocation.h"
#include "MFHomeAnchorComponent.h"
#include "MFLog.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTTask_GetHomeLocation::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_GetHomeLocation::EnterState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	const APawn* Pawn = Controller.GetPawn();
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	const UMFHomeAnchorComponent* Anchor = Pawn->FindComponentByClass<UMFHomeAnchorComponent>();
	if (!Anchor)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[STTask_GetHomeLocation] %s: UMFHomeAnchorComponent not found."),
			*GetNameSafe(Pawn));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.HomeLocation = Anchor->GetHomeLocation();
	return EStateTreeRunStatus::Succeeded;
}

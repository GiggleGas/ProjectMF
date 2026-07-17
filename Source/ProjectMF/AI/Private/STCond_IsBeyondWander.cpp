// Copyright ProjectMF. All Rights Reserved.

#include "STCond_IsBeyondWander.h"
#include "MFHomeAnchorComponent.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTCond_IsBeyondWander::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FSTCond_IsBeyondWander::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	const APawn* Pawn = Controller.GetPawn();
	if (!Pawn)
	{
		return false;
	}

	const UMFHomeAnchorComponent* Anchor = Pawn->FindComponentByClass<UMFHomeAnchorComponent>();
	return Anchor && Anchor->IsBeyondWander();
}

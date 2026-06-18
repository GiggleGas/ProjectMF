// Copyright ProjectMF. All Rights Reserved.

#include "STCond_CommandTypeIs.h"

#include "MFPetCommandComponent.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTCond_CommandTypeIs::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FSTCond_CommandTypeIs::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	const APawn* Pawn = Controller.GetPawn();
	if (!Pawn)
	{
		return false;
	}

	const UMFPetCommandComponent* CommandComp = Pawn->FindComponentByClass<UMFPetCommandComponent>();
	if (!CommandComp || !CommandComp->HasCommand())
	{
		return false;
	}

	return CommandComp->GetCommand().Type == InstanceData.ExpectedType;
}

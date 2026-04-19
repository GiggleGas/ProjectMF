// Copyright ProjectMF. All Rights Reserved.

#include "STCond_OwnerHasTag.h"
#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

// ============================================================================
// Link
// ============================================================================

bool FSTCond_OwnerHasTag::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

// ============================================================================
// TestCondition
// ============================================================================

bool FSTCond_OwnerHasTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	if (!InstanceData.TagToCheck.IsValid())
	{
		// Tag 未配置：视为条件不满足，避免空 Tag 导致意外的永真条件
		MF_LOG_WARNING(LogMFAI, TEXT("[STCond_OwnerHasTag] TagToCheck is not set — condition returns false."));
		return false;
	}

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	const AMFCharacterBase* Pawn    = Cast<AMFCharacterBase>(Controller.GetPawn());
	if (!Pawn)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Pawn->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(InstanceData.TagToCheck);
}

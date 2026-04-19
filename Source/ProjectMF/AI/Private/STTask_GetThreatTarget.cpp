// Copyright ProjectMF. All Rights Reserved.

#include "STTask_GetThreatTarget.h"
#include "MFThreatComponent.h"
#include "MFLog.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

// ============================================================================
// Link
// ============================================================================

bool FSTTask_GetThreatTarget::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

// ============================================================================
// EnterState
// ============================================================================

EStateTreeRunStatus FSTTask_GetThreatTarget::EnterState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	UMFThreatComponent* ThreatComp = GetThreatComp(Context);
	if (!ThreatComp)
	{
		const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
		MF_LOG_ERROR(LogMFAI,
			TEXT("[STTask_GetThreatTarget] %s: UMFThreatComponent not found on pawn."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = ThreatComp->GetCurrentTarget();
	InstanceData.TargetActor = Target;

	if (!Target)
	{
		// 进入时无目标：立即 Failed，StateTree 切换回 Idle 状态
		return EStateTreeRunStatus::Failed;
	}

	MF_LOG(LogMFAI, TEXT("[STTask_GetThreatTarget] %s: target acquired → %s"),
		*GetNameSafe(Context.GetExternalData(AIControllerHandle).GetPawn()),
		*Target->GetName());

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Tick
// ============================================================================

EStateTreeRunStatus FSTTask_GetThreatTarget::Tick(
	FStateTreeExecutionContext& Context,
	const float                DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	UMFThreatComponent* ThreatComp = GetThreatComp(Context);
	if (!ThreatComp)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = ThreatComp->GetCurrentTarget();

	// 目标切换时更新输出（正常情况，如目标死亡后切换到下一个）
	if (Target != InstanceData.TargetActor.Get())
	{
		InstanceData.TargetActor = Target;

		if (Target)
		{
			MF_LOG(LogMFAI, TEXT("[STTask_GetThreatTarget] %s: target changed → %s"),
				*GetNameSafe(Context.GetExternalData(AIControllerHandle).GetPawn()),
				*Target->GetName());
		}
	}

	// 无目标：返回 Failed，通知 StateTree 退出 Combat 状态
	if (!Target)
	{
		MF_LOG(LogMFAI, TEXT("[STTask_GetThreatTarget] %s: target lost → Failed."),
			*GetNameSafe(Context.GetExternalData(AIControllerHandle).GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// ExitState
// ============================================================================

void FSTTask_GetThreatTarget::ExitState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	InstanceData.TargetActor = nullptr;
}

// ============================================================================
// Private helper
// ============================================================================

UMFThreatComponent* FSTTask_GetThreatTarget::GetThreatComp(FStateTreeExecutionContext& Context) const
{
	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	APawn* Pawn = Controller.GetPawn();
	return Pawn ? Pawn->FindComponentByClass<UMFThreatComponent>() : nullptr;
}

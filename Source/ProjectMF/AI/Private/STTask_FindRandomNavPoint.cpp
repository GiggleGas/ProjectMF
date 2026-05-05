// Copyright ProjectMF. All Rights Reserved.

#include "STTask_FindRandomNavPoint.h"
#include "MFLog.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

// ============================================================================
// Link
// ============================================================================

bool FSTTask_FindRandomNavPoint::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

// ============================================================================
// EnterState
// ============================================================================

EStateTreeRunStatus FSTTask_FindRandomNavPoint::EnterState(
	FStateTreeExecutionContext&       Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	APawn* Pawn = Controller.GetPawn();
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld());
	if (!NavSys)
	{
		MF_LOG_ERROR(LogMFAI, TEXT("[FindRandomNavPoint] %s: NavigationSystem not found."),
			*GetNameSafe(Pawn));
		return EStateTreeRunStatus::Failed;
	}

	const FVector Origin = Pawn->GetActorLocation();
	const float MinR = FMath::Max(0.f, InstanceData.MinRadius);
	const float MaxR = FMath::Max(MinR, InstanceData.MaxRadius);

	// NavMesh 投影时的搜索容差（XY 方向用 MaxR，Z 方向固定容差）
	const FVector QueryExtent(200.f, 200.f, 200.f);

	FNavLocation NavLoc;
	for (int32 i = 0; i < InstanceData.MaxRetries; ++i)
	{
		// 环形区域面积均匀分布：r = sqrt(rand(MinR^2, MaxR^2))
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float R     = FMath::Sqrt(FMath::FRandRange(MinR * MinR, MaxR * MaxR));

		const FVector Candidate(
			Origin.X + R * FMath::Cos(Angle),
			Origin.Y + R * FMath::Sin(Angle),
			Origin.Z);

		if (NavSys->ProjectPointToNavigation(Candidate, NavLoc, QueryExtent))
		{
			InstanceData.ResultLocation = NavLoc.Location;

			MF_LOG(LogMFAI, TEXT("[FindRandomNavPoint] %s: found %s (r=%.0f, attempt %d/%d)"),
				*GetNameSafe(Pawn), *NavLoc.Location.ToString(), R,
				i + 1, InstanceData.MaxRetries);

			return EStateTreeRunStatus::Succeeded;
		}
	}

	MF_LOG_WARNING(LogMFAI,
		TEXT("[FindRandomNavPoint] %s: no NavMesh point found after %d retries (r=[%.0f, %.0f])."),
		*GetNameSafe(Pawn), InstanceData.MaxRetries, MinR, MaxR);

	return EStateTreeRunStatus::Failed;
}

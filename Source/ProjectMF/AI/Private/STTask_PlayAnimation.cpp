// Copyright ProjectMF. All Rights Reserved.

#include "STTask_PlayAnimation.h"

#include "MFCharacterBase.h"
#include "MFLog.h"

#include "PaperZDAnimInstance.h"
#include "PaperZDAnimationComponent.h"
#include "AnimSequences/PaperZDAnimSequence.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

// ============================================================================
// Link
// ============================================================================

bool FSTTask_PlayAnimation::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

// ============================================================================
// EnterState
// ============================================================================

EStateTreeRunStatus FSTTask_PlayAnimation::EnterState(
	FStateTreeExecutionContext&        Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& D = Context.GetInstanceData<FInstanceDataType>(*this);

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);

	if (!D.AnimSequence)
	{
		MF_LOG_ERROR(LogMFAbility,
			TEXT("[STTask_PlayAnimation] AnimSequence is null on %s — assign it in the StateTree editor."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	UPaperZDAnimInstance* AnimInst = GetAnimInstance(Context);
	if (!AnimInst)
	{
		MF_LOG_ERROR(LogMFAbility,
			TEXT("[STTask_PlayAnimation] Cannot get PaperZDAnimInstance from %s — "
			     "pawn must inherit AMFCharacterBase with a UPaperZDAnimationComponent."),
			*GetNameSafe(Controller.GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	D.AnimDuration = (D.PlayRate > KINDA_SMALL_NUMBER)
		? D.AnimSequence->GetTotalDuration() / D.PlayRate
		: 0.f;
	D.ElapsedTime = 0.f;

	AnimInst->PlayAnimationOverride(D.AnimSequence, D.SlotName, D.PlayRate);

	MF_LOG(LogMFAbility,
		TEXT("[STTask_PlayAnimation] %s → playing '%s'  duration=%.3fs  loop=%d  slot='%s'"),
		*GetNameSafe(Controller.GetPawn()),
		*GetNameSafe(D.AnimSequence),
		D.AnimDuration,
		D.bLoop ? 1 : 0,
		*D.SlotName.ToString());

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Tick
// ============================================================================

EStateTreeRunStatus FSTTask_PlayAnimation::Tick(
	FStateTreeExecutionContext& Context,
	const float                 DeltaTime) const
{
	FInstanceDataType& D = Context.GetInstanceData<FInstanceDataType>(*this);

	if (D.AnimDuration <= 0.f)
	{
		// 无法计算时长（序列异常）→ 直接放行
		return EStateTreeRunStatus::Succeeded;
	}

	D.ElapsedTime += DeltaTime;

	if (D.ElapsedTime < D.AnimDuration)
	{
		return EStateTreeRunStatus::Running;
	}

	if (D.bLoop)
	{
		// 循环：重置计时并重新播放
		D.ElapsedTime = 0.f;

		if (UPaperZDAnimInstance* AnimInst = GetAnimInstance(Context))
		{
			AnimInst->PlayAnimationOverride(D.AnimSequence, D.SlotName, D.PlayRate);
		}
		return EStateTreeRunStatus::Running;
	}

	// 单次播放结束
	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	MF_LOG(LogMFAbility, TEXT("[STTask_PlayAnimation] '%s' finished on %s → Succeeded."),
		*GetNameSafe(D.AnimSequence), *GetNameSafe(Controller.GetPawn()));

	return EStateTreeRunStatus::Succeeded;
}

// ============================================================================
// ExitState
// ============================================================================

void FSTTask_PlayAnimation::ExitState(
	FStateTreeExecutionContext&        Context,
	const FStateTreeTransitionResult& Transition) const
{
	// PaperZD Override 会被下一个 PlayAnimationOverride 或正常 AnimBP 状态自然覆盖，无需显式清除。
}

// ============================================================================
// Private helper
// ============================================================================

UPaperZDAnimInstance* FSTTask_PlayAnimation::GetAnimInstance(FStateTreeExecutionContext& Context) const
{
	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	AMFCharacterBase* Char = Cast<AMFCharacterBase>(Controller.GetPawn());
	if (!Char) return nullptr;

	UPaperZDAnimationComponent* AnimComp = Char->FindComponentByClass<UPaperZDAnimationComponent>();
	return AnimComp ? AnimComp->GetAnimInstance() : nullptr;
}

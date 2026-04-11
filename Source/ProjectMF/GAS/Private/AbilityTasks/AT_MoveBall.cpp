// Copyright ProjectMF. All Rights Reserved.

#include "AT_MoveBall.h"
#include "MFCatchBallActor.h"
#include "MFCatchPetConfig.h"
#include "MFLog.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

// ============================================================
// 工厂
// ============================================================

UAbilityTask_MoveBall* UAbilityTask_MoveBall::CreateMoveBall(
	UGameplayAbility*        OwningAbility,
	FName                    TaskInstanceName,
	ACatchBallActor*         InBall,
	AActor*                  InOwner,
	AActor*                  InTarget,
	const UMFCatchPetConfig* InConfig)
{
	UAbilityTask_MoveBall* Task =
		NewAbilityTask<UAbilityTask_MoveBall>(OwningAbility, TaskInstanceName);

	if (!InBall)   { MF_LOG_ERROR(LogMFCatch, TEXT("AT_MoveBall: InBall is null!")); }
	if (!InOwner)  { MF_LOG_ERROR(LogMFCatch, TEXT("AT_MoveBall: InOwner is null!")); }
	if (!InTarget) { MF_LOG_ERROR(LogMFCatch, TEXT("AT_MoveBall: InTarget is null!")); }
	if (!InConfig) { MF_LOG_ERROR(LogMFCatch, TEXT("AT_MoveBall: InConfig is null!")); }

	Task->BallActor   = InBall;
	Task->OwnerActor  = InOwner;
	Task->TargetActor = InTarget;
	Task->Config      = InConfig;

	return Task;
}

// ============================================================
// 生命周期
// ============================================================

void UAbilityTask_MoveBall::Activate()
{
	Super::Activate();

	if (!BallActor.IsValid() || !OwnerActor.IsValid() || !TargetActor.IsValid() || !Config)
	{
		MF_LOG_ERROR(LogMFCatch,
			TEXT("AT_MoveBall::Activate — missing dependency, ending task."));
		EndTask();
		return;
	}

	bTickingTask  = true;
	BallPhase     = EBallPhase::WaitingQTE;  // 从玩家侧开始：等待玩家按 Space 发球
	CurrentT      = 0.f;
	BounceCount   = 0;
	QTEElapsed    = 0.f;
	bWasSpaceDown = false;

	// 将球定位到玩家脚部，等待第一次发球
	BallActor->SetBallWorldLocation(GetOwnerFeetPos());

	// 通知 GA QTE 窗口开启（供 UI 显示提示）
	OnBallReachedPlayer.Broadcast();

	MF_LOG(LogMFCatch,
		TEXT("AT_MoveBall: Activated — waiting for first throw. Target=%s, MaxHits=%d, QTE=%.1fs."),
		*TargetActor->GetName(), Config->MaxBounceCount, Config->QTETimeLimit);
}

void UAbilityTask_MoveBall::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!IsValid(this) || !Config) return;

	// 检测目标/球已失效
	if (!BallActor.IsValid())
	{
		MF_LOG_ERROR(LogMFCatch, TEXT("AT_MoveBall: BallActor became invalid, ending task."));
		EndTask();
		return;
	}
	if (!OwnerActor.IsValid() || !TargetActor.IsValid())
	{
		MF_LOG_ERROR(LogMFCatch, TEXT("AT_MoveBall: Owner or Target became invalid, ending task."));
		EndTask();
		return;
	}

	const FVector FeetPos  = GetOwnerFeetPos();
	const FVector TargetPos = GetTargetPos();
	const float   Distance = FVector::Dist(FeetPos, TargetPos);

	// 距离近乎为零时跳过移动（防止 NaN）
	if (Distance < 1.f)
	{
		MF_LOG_WARNING(LogMFCatch, TEXT("AT_MoveBall: Owner and target too close (%.1f), skipping move."), Distance);
		return;
	}

	// T 每帧增量 = 实际速度（世界单位/秒）/ 当帧距离
	const float DeltaT = Config->BallSpeed * DeltaTime / Distance;

	// ---------------------------------------------------------------
	// 状态机
	// ---------------------------------------------------------------

	if (BallPhase == EBallPhase::MovingToPet)
	{
		CurrentT += DeltaT;
		if (CurrentT >= 1.f)
		{
			CurrentT = 1.f;

			// -------------------------------------------------------
			// 球抵达宠物：命中计数 + 成功判断
			// -------------------------------------------------------
			BounceCount++;
			MF_LOG(LogMFCatch,
				TEXT("AT_MoveBall: Ball hit pet! Hits %d / %d."),
				BounceCount, Config->MaxBounceCount);

			if (BounceCount >= Config->MaxBounceCount)
			{
				MF_LOG(LogMFCatch, TEXT("AT_MoveBall: All hits complete — catch succeeded!"));
				OnAllBouncesComplete.Broadcast();
				// GA 会调用 EndTask()
				return;
			}

			// 未达标：球自动弹回玩家
			BallPhase = EBallPhase::MovingToPlayer;
		}
	}
	else if (BallPhase == EBallPhase::MovingToPlayer)
	{
		CurrentT -= DeltaT;
		if (CurrentT <= 0.f)
		{
			CurrentT      = 0.f;
			BallPhase     = EBallPhase::WaitingQTE;
			QTEElapsed    = 0.f;
			bWasSpaceDown = false;

			MF_LOG(LogMFCatch,
				TEXT("AT_MoveBall: Ball back at player — QTE window open (%.1fs). Hits so far: %d / %d."),
				Config->QTETimeLimit, BounceCount, Config->MaxBounceCount);

			OnBallReachedPlayer.Broadcast();
		}
	}
	else // WaitingQTE — 玩家按 Space 将球发向宠物
	{
		QTEElapsed += DeltaTime;

		// --- Space 键边沿检测 ---
		bool bSpaceDown = false;
		if (Ability)
		{
			const FGameplayAbilityActorInfo* Info = Ability->GetCurrentActorInfo();
			if (Info && Info->PlayerController.IsValid())
			{
				bSpaceDown = Info->PlayerController->IsInputKeyDown(EKeys::SpaceBar);
			}
		}

		if (bSpaceDown && !bWasSpaceDown)
		{
			MF_LOG(LogMFCatch, TEXT("AT_MoveBall: Player threw the ball toward pet."));

			// 发球：切换到飞行状态（命中计数在球抵达宠物时才+1）
			BallPhase = EBallPhase::MovingToPet;
			OnQTESuccess.Broadcast();
		}
		else if (QTEElapsed >= Config->QTETimeLimit)
		{
			MF_LOG_WARNING(LogMFCatch,
				TEXT("AT_MoveBall: QTE timed out (%.1fs) — catch failed."),
				Config->QTETimeLimit);
			OnQTEFailed.Broadcast();
			// GA 会调用 EndTask()
			return;
		}

		bWasSpaceDown = bSpaceDown;
	}

	// ---------------------------------------------------------------
	// 更新球位置
	// ---------------------------------------------------------------
	BallActor->SetBallWorldLocation(ComputeBallPos(CurrentT));

#if ENABLE_DRAW_DEBUG
	// 单帧调试线：玩家脚部 → 宠物，球当前位置标记
	DrawDebugLine(GetWorld(), FeetPos, TargetPos, FColor::Orange, false, -1.f, 0, 1.f);
	DrawDebugSphere(GetWorld(), ComputeBallPos(CurrentT), 12.f, 6,
		BallPhase == EBallPhase::WaitingQTE ? FColor::Yellow : FColor::White,
		false, -1.f);
#endif
}

void UAbilityTask_MoveBall::OnDestroy(bool bInOwnerFinished)
{
	MF_LOG(LogMFCatch,
		TEXT("AT_MoveBall: OnDestroy — BounceCount=%d, Phase=%d."),
		BounceCount, static_cast<int32>(BallPhase));

	Super::OnDestroy(bInOwnerFinished);
}

// ============================================================
// 位置辅助
// ============================================================

FVector UAbilityTask_MoveBall::GetOwnerFeetPos() const
{
	AActor* Owner = OwnerActor.Get();
	if (!Owner) return FVector::ZeroVector;

	FVector Pos = Owner->GetActorLocation();
	if (const ACharacter* Char = Cast<ACharacter>(Owner))
	{
		Pos.Z -= Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	return Pos;
}

FVector UAbilityTask_MoveBall::GetTargetPos() const
{
	AActor* Target = TargetActor.Get();
	return Target ? Target->GetActorLocation() : FVector::ZeroVector;
}

FVector UAbilityTask_MoveBall::ComputeBallPos(float T) const
{
	return FMath::Lerp(GetOwnerFeetPos(), GetTargetPos(), T);
}

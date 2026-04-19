// Copyright ProjectMF. All Rights Reserved.

#include "MFThreatComponent.h"
#include "MFRadarSensingComponent.h"
#include "MFGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "MFLog.h"

// ============================================================
// CVar: MF.Debug.ThreatSystem
// 控制台输入: MF.Debug.ThreatSystem 1  开启 / 0  关闭
// 开启后每次 PeriodicUpdate 绘制：
//   - 绿色球体    = 交战半径（Idle）
//   - 红色球体    = 交战半径（Locked，目标在感知范围内）
//   - 黄色球体    = 交战半径（Locked，目标在宽限期内）
//   - 橙色粗箭头  = AI → 当前目标
//   - AI 头顶文字 = [状态] Target: X  (in range / grace: Xs)
//   - 威胁列表    = 每个威胁头顶小字：距离 / 宽限剩余
// ============================================================

static int32 GMFDebugThreatSystem = 0;
static FAutoConsoleVariableRef CVarMFDebugThreatSystem(
	TEXT("MF.Debug.ThreatSystem"),
	GMFDebugThreatSystem,
	TEXT("Show AI threat/targeting system debug visualization.\n")
	TEXT("  1 = on  (sphere=engagement range, arrow=AI->target, text=state/threat list)\n")
	TEXT("  0 = off (default)\n")
	TEXT("Usage: MF.Debug.ThreatSystem 1"),
	ECVF_Default
);

// ============================================================
// 构造
// ============================================================

UMFThreatComponent::UMFThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================
// 生命周期
// ============================================================

void UMFThreatComponent::BeginPlay()
{
	Super::BeginPlay();

	RadarComp = GetOwner()->FindComponentByClass<UMFRadarSensingComponent>();
	if (!RadarComp)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Threat] %s: UMFRadarSensingComponent not found."),
			*GetOwner()->GetName());
		return;
	}

	RadarComp->OnTargetDetected.AddDynamic(this, &UMFThreatComponent::HandleTargetDetected);
	RadarComp->OnTargetLost.AddDynamic(this, &UMFThreatComponent::HandleTargetLost);

	RestartEvalTimer();
}

void UMFThreatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvalTimerHandle);

		// 清理所有威胁记录的 GraceTimer
		for (FMFThreatRecord& Record : ThreatRecords)
		{
			World->GetTimerManager().ClearTimer(Record.GraceTimer);
		}
	}
	ThreatRecords.Empty();

	if (RadarComp)
	{
		RadarComp->OnTargetDetected.RemoveDynamic(this, &UMFThreatComponent::HandleTargetDetected);
		RadarComp->OnTargetLost.RemoveDynamic(this, &UMFThreatComponent::HandleTargetLost);
	}

	Super::EndPlay(EndPlayReason);
}

// ============================================================
// 配置
// ============================================================

void UMFThreatComponent::ApplyConfig(const FMFThreatConfig& InConfig)
{
	Config = InConfig;

	if (RadarComp && Config.EngagementRadius > RadarComp->SensingRadius)
	{
		MF_LOG_WARNING(LogMFAI,
			TEXT("[Threat] %s: EngagementRadius (%.0f) > SensingRadius (%.0f), clamping."),
			*GetOwner()->GetName(), Config.EngagementRadius, RadarComp->SensingRadius);
		Config.EngagementRadius = RadarComp->SensingRadius;
	}

	RestartEvalTimer();

	MF_LOG(LogMFAI,
		TEXT("[Threat] %s: Config applied — Engagement=%.0f, GraceDuration=%.1fs, EvalInterval=%.2fs"),
		*GetOwner()->GetName(), Config.EngagementRadius, Config.LockDuration, Config.EvaluationInterval);
}

// ============================================================
// 查询接口
// ============================================================

AActor* UMFThreatComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}

bool UMFThreatComponent::HasTarget() const
{
	return CurrentTarget.IsValid();
}

float UMFThreatComponent::GetCurrentTargetGraceTime() const
{
	if (State != EMFThreatState::Locked) { return 0.f; }

	const FMFThreatRecord* Record = FindRecord(CurrentTarget.Get());
	if (!Record || !Record->GraceTimer.IsValid()) { return 0.f; }

	const UWorld* World = GetWorld();
	return World ? World->GetTimerManager().GetTimerRemaining(Record->GraceTimer) : 0.f;
}

TArray<AActor*> UMFThreatComponent::GetThreatListActors() const
{
	TArray<AActor*> Result;
	Result.Reserve(ThreatRecords.Num());
	for (const FMFThreatRecord& Record : ThreatRecords)
	{
		if (AActor* Actor = Record.Actor.Get())
		{
			Result.Add(Actor);
		}
	}
	return Result;
}

// ============================================================
// 周期性更新
// ============================================================

void UMFThreatComponent::RestartEvalTimer()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	World->GetTimerManager().ClearTimer(EvalTimerHandle);
	if (Config.EvaluationInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(
			EvalTimerHandle,
			this, &UMFThreatComponent::PeriodicUpdate,
			Config.EvaluationInterval, /*bLoop=*/true, /*FirstDelay=*/Config.EvaluationInterval);
	}
}

void UMFThreatComponent::PeriodicUpdate()
{
	CleanupThreatList();

	const FVector MyLoc = GetOwner()->GetActorLocation();

	// --- 宽限期目标的 EngagementRadius 回归检测 ---
	// 目标可能在宽限期内重新进入 EngagementRadius（但尚未到达 SensingRadius，雷达不会触发事件）。
	// 此时取消宽限计时器：目标重新进入交战范围，AI 继续/重新锁定，无需计时。
	for (FMFThreatRecord& Record : ThreatRecords)
	{
		// 只处理当前处于宽限期的目标（bInSensingRange=false 且 GraceTimer 在跑）
		if (Record.bInSensingRange || !Record.GraceTimer.IsValid()) { continue; }

		AActor* Actor = Record.Actor.Get();
		if (!Actor) { continue; }

		const float Dist = FVector::Dist(MyLoc, Actor->GetActorLocation());
		if (Dist <= Config.EngagementRadius)
		{
			// 目标回到交战范围：取消宽限，保持在威胁列表中
			GetWorld()->GetTimerManager().ClearTimer(Record.GraceTimer);

			MF_LOG(LogMFAI, TEXT("[Threat] %s -> %s returned to EngagementRadius (%.0f), grace cancelled."),
				*GetOwner()->GetName(), *Actor->GetName(), Dist);

			// 若当前为 Idle（例如之前目标被移除），立即重新评估
			if (State == EMFThreatState::Idle)
			{
				EvaluateTargets();
				break;  // EvaluateTargets 可能改变 State，跳出循环
			}
			// 若 Locked 且此目标就是 CurrentTarget：无需操作，锁定已维持
			// 若 Locked 但此目标不是 CurrentTarget：下次 EvalTimer 或事件驱动时再评估
		}
	}

	// --- 状态机兜底检查 ---
	switch (State)
	{
	case EMFThreatState::Locked:
		// 兜底：CurrentTarget 已不在威胁列表（极端情况：Actor 销毁但 GraceTimer 未触发）
		if (!CurrentTarget.IsValid() || !FindRecord(CurrentTarget.Get()))
		{
			MF_LOG(LogMFAI, TEXT("[Threat] %s -> PeriodicUpdate: target gone, re-evaluating."),
				*GetOwner()->GetName());
			EvaluateTargets();
		}
		break;

	case EMFThreatState::Idle:
		// 主动评估：威胁列表中可能已有在 EngagementRadius 内的目标
		if (!ThreatRecords.IsEmpty())
		{
			EvaluateTargets();
		}
		break;
	}

#if ENABLE_DRAW_DEBUG
	if (GMFDebugThreatSystem)
	{
		DrawDebugVisualization();
	}
#endif
}

// ============================================================
// 状态机
// ============================================================

void UMFThreatComponent::TransitionTo(EMFThreatState NewState, AActor* InTarget)
{
	// 退出旧状态（当前设计：Locked 无组件级 Timer 需清理，GraceTimer 在记录上）
	State = NewState;

	switch (State)
	{
	case EMFThreatState::Idle:   EnterIdle();           break;
	case EMFThreatState::Locked: EnterLocked(InTarget); break;
	}
}

void UMFThreatComponent::EnterIdle()
{
	SetCurrentTarget(nullptr);
	MF_LOG(LogMFAI, TEXT("[Threat] %s -> State: Idle"), *GetOwner()->GetName());
}

void UMFThreatComponent::EnterLocked(AActor* Target)
{
	SetCurrentTarget(Target);
	// 不启动组件级 LockTimer：
	// - 目标在感知范围内 → 无限持有（无需计时）
	// - 目标离开感知范围 → GraceTimer 在 HandleTargetLost 中启动
	MF_LOG(LogMFAI, TEXT("[Threat] %s -> State: Locked | Target: %s"),
		*GetOwner()->GetName(), Target ? *Target->GetName() : TEXT("null"));
}

// ============================================================
// 目标评估
// ============================================================

void UMFThreatComponent::EvaluateTargets()
{
	CleanupThreatList();

	const FVector MyLoc    = GetOwner()->GetActorLocation();
	AActor*       Best     = nullptr;
	float         BestScore = -BIG_NUMBER;

	for (const FMFThreatRecord& Record : ThreatRecords)
	{
		AActor* Actor = Record.Actor.Get();
		if (!Actor) { continue; }

		const float Score = ScoreTarget(Actor, MyLoc);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best      = Actor;
		}
	}

	if (Best)
	{
		TransitionTo(EMFThreatState::Locked, Best);
	}
	else
	{
		TransitionTo(EMFThreatState::Idle);
	}
}

float UMFThreatComponent::ScoreTarget(AActor* Target, const FVector& OwnerLoc) const
{
	if (!Target) { return -BIG_NUMBER; }

	const float Dist = FVector::Dist(OwnerLoc, Target->GetActorLocation());

	// 必须在 EngagementRadius 内才能被选为目标
	if (Dist > Config.EngagementRadius) { return -BIG_NUMBER; }

	// 越近分越高（主要判定：距离）
	// 扩展点：可在此叠加 Boss 权重、仇恨值、HP 百分比等
	return Config.EngagementRadius - Dist;
}

void UMFThreatComponent::SetCurrentTarget(AActor* NewTarget)
{
	AActor* OldTarget = CurrentTarget.Get();
	if (NewTarget == OldTarget) { return; }

	CurrentTarget = NewTarget;

	if (UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (NewTarget)
			ASC->AddLooseGameplayTag(MFGameplayTags::AI_Perception_HasTarget);
		else
			ASC->RemoveLooseGameplayTag(MFGameplayTags::AI_Perception_HasTarget);
	}

	OnTargetChanged.Broadcast(NewTarget);
}

// ============================================================
// 威胁列表管理
// ============================================================

FMFThreatRecord* UMFThreatComponent::FindRecord(const AActor* Actor)
{
	for (FMFThreatRecord& Record : ThreatRecords)
	{
		if (Record.Actor.Get() == Actor) { return &Record; }
	}
	return nullptr;
}

const FMFThreatRecord* UMFThreatComponent::FindRecord(const AActor* Actor) const
{
	for (const FMFThreatRecord& Record : ThreatRecords)
	{
		if (Record.Actor.Get() == Actor) { return &Record; }
	}
	return nullptr;
}

void UMFThreatComponent::CleanupThreatList()
{
	UWorld* World = GetWorld();
	for (int32 i = ThreatRecords.Num() - 1; i >= 0; --i)
	{
		if (!ThreatRecords[i].Actor.IsValid())
		{
			if (World) { World->GetTimerManager().ClearTimer(ThreatRecords[i].GraceTimer); }
			ThreatRecords.RemoveAt(i);
		}
	}
}

void UMFThreatComponent::OnGraceExpired(TWeakObjectPtr<AActor> WeakActor)
{
	AActor* Actor = WeakActor.Get();
	const bool bWasCurrentTarget = (Actor && Actor == CurrentTarget.Get())
		|| (!Actor && !CurrentTarget.IsValid());  // 两者都已失效

	// 从威胁列表移除
	ThreatRecords.RemoveAll([&WeakActor](const FMFThreatRecord& R)
	{
		return R.Actor == WeakActor;
	});

	MF_LOG(LogMFAI, TEXT("[Threat] %s -> Grace expired: %s removed from threat list."),
		*GetOwner()->GetName(), Actor ? *Actor->GetName() : TEXT("(destroyed)"));

	// 若移除的是当前目标，立即重新评估
	if (bWasCurrentTarget || CurrentTarget.Get() == Actor)
	{
		EvaluateTargets();
	}
}

// ============================================================
// RadarSensing 事件处理
// ============================================================

void UMFThreatComponent::HandleTargetDetected(AActor* Target)
{
	if (!Target) { return; }

	FMFThreatRecord* Existing = FindRecord(Target);
	if (Existing)
	{
		// 目标重新进入感知范围：取消宽限计时器，恢复"在范围内"状态
		if (Existing->GraceTimer.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(Existing->GraceTimer);

			MF_LOG(LogMFAI, TEXT("[Threat] %s -> %s returned to sensing range, grace timer cancelled."),
				*GetOwner()->GetName(), *Target->GetName());
		}
		Existing->bInSensingRange = true;
	}
	else
	{
		// 新威胁：加入列表
		FMFThreatRecord NewRecord;
		NewRecord.Actor          = Target;
		NewRecord.bInSensingRange = true;
		ThreatRecords.Add(NewRecord);

		MF_LOG(LogMFAI, TEXT("[Threat] %s -> New threat: %s added to threat list (%d total)."),
			*GetOwner()->GetName(), *Target->GetName(), ThreatRecords.Num());
	}

	// Idle 时立即评估（Locked 时由 GraceTimer 或 PeriodicUpdate 处理）
	if (State == EMFThreatState::Idle)
	{
		EvaluateTargets();
	}
}

void UMFThreatComponent::HandleTargetLost(AActor* Target)
{
	if (!Target) { return; }

	FMFThreatRecord* Record = FindRecord(Target);
	if (!Record) { return; }

	Record->bInSensingRange = false;

	// 启动宽限计时器：宽限期内 AI 仍可追击目标（保持锁定）
	// 宽限期满 → OnGraceExpired → 从威胁列表移除 → 若是当前目标则重新评估
	TWeakObjectPtr<AActor> WeakTarget(Target);
	FTimerDelegate GraceDelegate;
	GraceDelegate.BindLambda([this, WeakTarget]()
	{
		OnGraceExpired(WeakTarget);
	});
	GetWorld()->GetTimerManager().SetTimer(
		Record->GraceTimer, GraceDelegate, Config.LockDuration, /*bLoop=*/false);

	MF_LOG(LogMFAI, TEXT("[Threat] %s -> %s left sensing range, grace timer started (%.1fs)."),
		*GetOwner()->GetName(), *Target->GetName(), Config.LockDuration);

	// 注意：此处不立即重新评估——宽限期内保持锁定，让 AI 可以追击
}

// ============================================================
// Debug 可视化
// ============================================================

void UMFThreatComponent::DrawDebugVisualization() const
{
#if ENABLE_DRAW_DEBUG
	if (!GMFDebugThreatSystem) { return; }

	const UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	if (!World || !Owner) { return; }

	const FVector OwnerLoc     = Owner->GetActorLocation();
	const float   DrawDuration = Config.EvaluationInterval * 1.5f;

	// --- 1. 交战半径球体 ---
	// Idle=绿；Locked+目标在感知范围=红；Locked+目标在宽限期=黄
	FColor SphereColor = FColor::Green;
	if (State == EMFThreatState::Locked)
	{
		const FMFThreatRecord* Record = FindRecord(CurrentTarget.Get());
		SphereColor = (Record && Record->bInSensingRange) ? FColor::Red : FColor::Yellow;
	}
	DrawDebugSphere(World, OwnerLoc, Config.EngagementRadius,
		/*Segments=*/24, SphereColor, false, DrawDuration, 0, 2.5f);

	// --- 2. AI → 当前目标 箭头（Locked 时）---
	if (State == EMFThreatState::Locked)
	{
		if (const AActor* Target = CurrentTarget.Get())
		{
			DrawDebugDirectionalArrow(World, OwnerLoc, Target->GetActorLocation(),
				100.f, FColor::Orange, false, DrawDuration, 0, 3.f);
		}
	}

	// --- 3. AI 头顶：状态 + 目标 + 宽限信息 ---
	{
		FString StatusText;
		if (State == EMFThreatState::Locked && CurrentTarget.IsValid())
		{
			const FMFThreatRecord* Record = FindRecord(CurrentTarget.Get());
			const bool bInRange = Record && Record->bInSensingRange;
			if (bInRange)
			{
				StatusText = FString::Printf(TEXT("[LOCKED] %s  (in range)"),
					*CurrentTarget->GetName());
			}
			else
			{
				const float Grace = GetCurrentTargetGraceTime();
				StatusText = FString::Printf(TEXT("[LOCKED] %s  (grace: %.1fs)"),
					*CurrentTarget->GetName(), Grace);
			}
		}
		else
		{
			StatusText = FString::Printf(TEXT("[IDLE]  threats: %d"), ThreatRecords.Num());
		}

		DrawDebugString(World, OwnerLoc + FVector(0.f, 0.f, 120.f), StatusText,
			nullptr,
			(State == EMFThreatState::Locked) ? FColor::Orange : FColor::Green,
			DrawDuration, true);
	}

	// --- 4. 威胁列表：每个威胁头顶显示距离 + 范围状态 ---
	// 图例：[engage] = 在交战范围内可被选目标；[sense] = 在感知范围内但超出交战范围；[grace:Xs] = 宽限期
	for (const FMFThreatRecord& Record : ThreatRecords)
	{
		const AActor* ThreatActor = Record.Actor.Get();
		if (!ThreatActor) { continue; }

		const float Dist     = FVector::Dist(OwnerLoc, ThreatActor->GetActorLocation());
		const bool  bCurrent = (ThreatActor == CurrentTarget.Get());
		const bool  bEngaged = (Dist <= Config.EngagementRadius);

		FString Info;
		if (Record.bInSensingRange)
		{
			// 在感知范围内，区分是否在交战范围
			Info = FString::Printf(TEXT("%.0fm  [%s]%s"),
				Dist / 100.f,
				bEngaged ? TEXT("engage") : TEXT("sense"),
				bCurrent ? TEXT(" ★") : TEXT(""));
		}
		else
		{
			// 宽限期中（已离开感知范围）
			const float GraceLeft = Record.GraceTimer.IsValid()
				? World->GetTimerManager().GetTimerRemaining(Record.GraceTimer)
				: 0.f;
			Info = FString::Printf(TEXT("%.0fm  [grace: %.1fs]%s"),
				Dist / 100.f, GraceLeft, bCurrent ? TEXT(" ★") : TEXT(""));
		}

		const FColor InfoColor = bCurrent ? FColor::Orange : (bEngaged ? FColor::White : FColor(160, 160, 160));
		DrawDebugString(World, ThreatActor->GetActorLocation() + FVector(0.f, 0.f, 50.f),
			Info, nullptr, InfoColor, DrawDuration, false);
	}
#endif // ENABLE_DRAW_DEBUG
}

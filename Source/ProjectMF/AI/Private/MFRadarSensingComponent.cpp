// Copyright ProjectMF. All Rights Reserved.

#include "MFRadarSensingComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "MFLog.h"
#include "Engine/OverlapResult.h"

// ============================================================
// CVar: MF.Debug.RadarSensing
// 控制台输入: MF.Debug.RadarSensing 1  开启 / 0  关闭
// 开启后每次扫描时绘制：
//   - 青色球体  = 感知范围（无目标时）
//   - 黄色球体  = 感知范围（有目标时）
//   - 红色箭头  = 从被感知目标指向感知到它的 AI
// ============================================================

static int32 GMFDebugRadarSensing = 0;
static FAutoConsoleVariableRef CVarMFDebugRadarSensing(
	TEXT("MF.Debug.RadarSensing"),
	GMFDebugRadarSensing,
	TEXT("Show AI radar sensing debug visualization.\n")
	TEXT("  1 = on  (cyan sphere = range, yellow = range with targets, red arrows = target→AI)\n")
	TEXT("  0 = off (default)\n")
	TEXT("Usage: MF.Debug.RadarSensing 1"),
	ECVF_Default
);

// ============================================================
// 构造
// ============================================================

UMFRadarSensingComponent::UMFRadarSensingComponent()
{
	// 不需要每帧 Tick，使用定时器驱动扫描。
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================
// 生命周期
// ============================================================

void UMFRadarSensingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ScanInterval > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ScanTimerHandle,
			this, &UMFRadarSensingComponent::PerformScan,
			ScanInterval,
			/*bLoop=*/true,
			/*FirstDelay=*/ScanInterval
		);
	}
}

void UMFRadarSensingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	// 对所有仍在感知列表中的有效目标广播 OnTargetLost，保证外部状态一致。
	for (const TWeakObjectPtr<AActor>& Weak : PerceivedActors)
	{
		if (AActor* Actor = Weak.Get())
		{
			OnTargetLost.Broadcast(Actor);
		}
	}
	PerceivedActors.Empty();

	Super::EndPlay(EndPlayReason);
}

// ============================================================
// 查询接口
// ============================================================

TArray<AActor*> UMFRadarSensingComponent::GetPerceivedActors() const
{
	TArray<AActor*> Result;
	Result.Reserve(PerceivedActors.Num());
	for (const TWeakObjectPtr<AActor>& Weak : PerceivedActors)
	{
		if (AActor* Actor = Weak.Get())
		{
			Result.Add(Actor);
		}
	}
	return Result;
}

bool UMFRadarSensingComponent::IsActorPerceived(AActor* Actor) const
{
	if (!Actor) { return false; }
	return PerceivedActors.Contains(TWeakObjectPtr<AActor>(Actor));
}

int32 UMFRadarSensingComponent::GetPerceivedCount() const
{
	// 过滤掉已销毁的弱引用再计数（保持准确性）。
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Weak : PerceivedActors)
	{
		if (Weak.IsValid()) { ++Count; }
	}
	return Count;
}

// ============================================================
// 运行时控制
// ============================================================

void UMFRadarSensingComponent::ApplyConfig(const FMFRadarSensingConfig& InConfig)
{
	SensingRadius = InConfig.SensingRadius;
	TargetTags    = InConfig.TargetTags;

	// ScanInterval 变化时需重启 Timer；Radius / Tags 直接写入即可（下次扫描自动生效）。
	if (!FMath::IsNearlyEqual(InConfig.ScanInterval, ScanInterval) || !ScanTimerHandle.IsValid())
	{
		ScanInterval = InConfig.ScanInterval;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ScanTimerHandle);
			if (ScanInterval > 0.f)
			{
				World->GetTimerManager().SetTimer(
					ScanTimerHandle,
					this, &UMFRadarSensingComponent::PerformScan,
					ScanInterval, /*bLoop=*/true, /*FirstDelay=*/ScanInterval);
			}
		}
	}

	MF_LOG(LogMFAI, TEXT("[RadarSensing] %s: Config applied — Radius=%.0f, Interval=%.2fs, Tags=%s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		SensingRadius, ScanInterval, *TargetTags.ToStringSimple());
}

void UMFRadarSensingComponent::ForceScan()
{
	PerformScan();
}

// ============================================================
// 核心扫描逻辑
// ============================================================

void UMFRadarSensingComponent::PerformScan()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) { return; }

	const FVector OwnerLocation = Owner->GetActorLocation();

	// --- 1. 球形重叠，获取当前帧范围内的所有候选 ---
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MFRadarScan), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(Owner);

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		OwnerLocation,
		FQuat::Identity,
		SensingChannel,
		FCollisionShape::MakeSphere(SensingRadius),
		QueryParams
	);

	// --- 2. 收集本次扫描中通过过滤的 Actor ---
	TSet<TWeakObjectPtr<AActor>> CurrentFrame;
	for (const FOverlapResult& Hit : Overlaps)
	{
		AActor* Candidate = Hit.GetActor();
		if (!Candidate || Candidate == Owner) { continue; }
		if (!PassesTagFilter(Candidate)) { continue; }

		CurrentFrame.Add(TWeakObjectPtr<AActor>(Candidate));
	}

	// --- 3. 新进入范围的目标：在 CurrentFrame 中但不在 PerceivedActors 中 ---
	for (const TWeakObjectPtr<AActor>& Weak : CurrentFrame)
	{
		if (!PerceivedActors.Contains(Weak))
		{
			PerceivedActors.Add(Weak);
			if (AActor* Actor = Weak.Get())
			{
				MF_LOG(LogMFAI, TEXT("[RadarSensing] %s -> Detected: %s"),
					*Owner->GetName(), *Actor->GetName());
				OnTargetDetected.Broadcast(Actor);
			}
		}
	}

	// --- 4. 离开范围的目标：在 PerceivedActors 中但不在 CurrentFrame 中，或已销毁 ---
	TArray<TWeakObjectPtr<AActor>> ToRemove;
	for (const TWeakObjectPtr<AActor>& Weak : PerceivedActors)
	{
		if (!Weak.IsValid() || !CurrentFrame.Contains(Weak))
		{
			ToRemove.Add(Weak);
		}
	}
	for (const TWeakObjectPtr<AActor>& Weak : ToRemove)
	{
		PerceivedActors.Remove(Weak);
		if (AActor* Actor = Weak.Get())
		{
			MF_LOG(LogMFAI, TEXT("[RadarSensing] %s -> Lost: %s"),
				*Owner->GetName(), *Actor->GetName());
			OnTargetLost.Broadcast(Actor);
		}
	}

	// --- 5. Debug 可视化（CVar MF.Debug.RadarSensing 1 开启）---
	DrawDebugVisualization(OwnerLocation);
}

void UMFRadarSensingComponent::DrawDebugVisualization(const FVector& OwnerLocation) const
{
#if ENABLE_DRAW_DEBUG
	if (!GMFDebugRadarSensing) { return; }

	UWorld* World = GetWorld();
	if (!World) { return; }

	// 绘制持续时间略长于扫描间隔，确保两次扫描之间不会闪烁。
	const float DrawDuration = ScanInterval * 1.5f;

	const bool bHasTargets = (PerceivedActors.Num() > 0);

	// 感知范围球体：无目标时青色，有目标时黄色。
	const FColor SphereColor = bHasTargets ? FColor::Yellow : FColor::Cyan;
	DrawDebugSphere(World, OwnerLocation, SensingRadius, /*Segments=*/32, SphereColor,
		/*bPersistent=*/false, DrawDuration, /*DepthPriority=*/0, /*Thickness=*/1.5f);

	// 从每个被感知目标位置画箭头，指向本 AI（"谁感知了我"）。
	for (const TWeakObjectPtr<AActor>& Weak : PerceivedActors)
	{
		const AActor* Target = Weak.Get();
		if (!Target) { continue; }

		const FVector TargetLoc = Target->GetActorLocation();
		DrawDebugDirectionalArrow(
			World,
			TargetLoc,           // 起点：被感知目标
			OwnerLocation,       // 终点：感知到它的 AI
			/*ArrowSize=*/80.f,
			FColor::Red,
			/*bPersistent=*/false,
			DrawDuration,
			/*DepthPriority=*/0,
			/*Thickness=*/2.f
		);

		// 在目标头顶标注 AI 的名字，方便多个 AI 同时感知同一目标时区分。
		const AActor* Owner = GetOwner();
		if (Owner)
		{
			DrawDebugString(
				World,
				TargetLoc + FVector(0.f, 0.f, 80.f),
				FString::Printf(TEXT("→ %s"), *Owner->GetName()),
				/*TestBaseActor=*/nullptr,
				FColor::Orange,
				DrawDuration,
				/*bDrawShadow=*/true
			);
		}
	}
#endif // ENABLE_DRAW_DEBUG
}

// ============================================================
// 标签过滤
// ============================================================

bool UMFRadarSensingComponent::PassesTagFilter(AActor* Candidate) const
{
	// TargetTags 为空时不过滤：接受所有重叠 Pawn。
	if (TargetTags.IsEmpty()) { return true; }

	// 通过 AbilitySystemGlobals 获取目标 ASC（空安全）。
	const UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate);

	if (!TargetASC) { return false; }

	// 目标 ASC 拥有的标签中只要有一个在 TargetTags 里，就通过过滤。
	return TargetASC->HasAnyMatchingGameplayTags(TargetTags);
}

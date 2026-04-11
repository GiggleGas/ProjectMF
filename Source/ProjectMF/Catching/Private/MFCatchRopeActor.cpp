// Copyright ProjectMF. All Rights Reserved.

#include "MFCatchRopeActor.h"
#include "MFLog.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

ACatchRopeActor::ACatchRopeActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Spline 作为根组件，端点在 SetEndpoints/Tick 中更新
	RopeSpline = CreateDefaultSubobject<USplineComponent>(TEXT("RopeSpline"));
	SetRootComponent(RopeSpline);

	// 初始化两个控制点（位置由 UpdateSplinePoints 填充）
	RopeSpline->ClearSplinePoints(false);
	RopeSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, false);
	RopeSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, true);
}

void ACatchRopeActor::BeginPlay()
{
	Super::BeginPlay();
	MF_LOG(LogMFCatch, TEXT("ACatchRopeActor: Spawned."));
}

void ACatchRopeActor::SetEndpoints(AActor* InStart, AActor* InEnd)
{
	if (!InStart)
	{
		MF_LOG_ERROR(LogMFCatch, TEXT("ACatchRopeActor::SetEndpoints — StartActor is null!"));
	}
	if (!InEnd)
	{
		MF_LOG_ERROR(LogMFCatch, TEXT("ACatchRopeActor::SetEndpoints — EndActor is null!"));
	}

	StartActor = InStart;
	EndActor   = InEnd;
	UpdateSplinePoints();

	MF_LOG(LogMFCatch, TEXT("ACatchRopeActor: Endpoints set — Start=%s, End=%s"),
		InStart ? *InStart->GetName() : TEXT("null"),
		InEnd   ? *InEnd->GetName()   : TEXT("null"));
}

void ACatchRopeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 端点 Actor 有效时每帧跟踪位置
	if (StartActor.IsValid() && EndActor.IsValid())
	{
		UpdateSplinePoints();
	}
	else
	{
		// 任意一端失效（Actor 被销毁）则自毁
		if (!StartActor.IsValid())
		{
			MF_LOG_WARNING(LogMFCatch, TEXT("ACatchRopeActor: StartActor became invalid, destroying rope."));
		}
		if (!EndActor.IsValid())
		{
			MF_LOG_WARNING(LogMFCatch, TEXT("ACatchRopeActor: EndActor became invalid, destroying rope."));
		}
		Destroy();
	}
}

void ACatchRopeActor::UpdateSplinePoints()
{
	if (!RopeSpline) return;

	// 起点与 AT_MoveBall 保持一致：角色脚底（减去胶囊体半高）
	FVector StartPos = FVector::ZeroVector;
	if (StartActor.IsValid())
	{
		StartPos = StartActor->GetActorLocation();
		if (const ACharacter* Char = Cast<ACharacter>(StartActor.Get()))
		{
			StartPos.Z -= Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
	}

	const FVector EndPos = EndActor.IsValid()
		? EndActor->GetActorLocation()
		: FVector::ZeroVector;

	RopeSpline->SetLocationAtSplinePoint(0, StartPos, ESplineCoordinateSpace::World, false);
	RopeSpline->SetLocationAtSplinePoint(1, EndPos,   ESplineCoordinateSpace::World, true);

	// 原型：DrawDebugLine 显示绳索，后期替换为 SplineMeshComponent
#if ENABLE_DRAW_DEBUG
	if (const UWorld* World = GetWorld())
	{
		DrawDebugLine(World, StartPos, EndPos,
			FColor::Orange, false, -1.f, 0, 3.f);
	}
#endif
}

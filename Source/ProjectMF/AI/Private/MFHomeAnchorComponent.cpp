// Copyright ProjectMF. All Rights Reserved.

#include "MFHomeAnchorComponent.h"
#include "MFLog.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

// ---------------------------------------------------------------------------
// CVar：MF.Debug.HomeAnchor 1 → 画锚点(十字) + 游离半径(绿圈) + owner→home 连线
// ---------------------------------------------------------------------------
static int32 GHomeAnchorDebug = 0;
static FAutoConsoleVariableRef CVarHomeAnchorDebug(
	TEXT("MF.Debug.HomeAnchor"),
	GHomeAnchorDebug,
	TEXT("Draw AI home anchor cross + wander radius circle + owner→home line. 1=on, 0=off."),
	ECVF_Cheat);

UMFHomeAnchorComponent::UMFHomeAnchorComponent()
{
	// 逻辑纯查询式无需 tick；仅在开 debug cvar 时用 tick 画可视化。
	PrimaryComponentTick.bCanEverTick = true;
}

void UMFHomeAnchorComponent::BeginPlay()
{
	Super::BeginPlay();

	// 出生即记家：SpawnManager 生成的 AI（Spawn 到点后 BeginPlay）与关卡手摆的 AI 都在此自记。
	if (const AActor* Owner = GetOwner())
	{
		SetHome(Owner->GetActorLocation());
	}
}

void UMFHomeAnchorComponent::ApplyConfig(const FMFHomeAnchorConfig& InConfig)
{
	Config = InConfig;
}

void UMFHomeAnchorComponent::SetHome(const FVector& Loc)
{
	HomeLocation = Loc;
	bHomeSet     = true;

	const AActor* Owner = GetOwner();
	UE_LOG(LogMFAI, Log, TEXT("[HomeAnchor] %s: home set at (%.0f, %.0f, %.0f)  WanderR=%.0f"),
		Owner ? *Owner->GetName() : TEXT("?"), Loc.X, Loc.Y, Loc.Z, Config.WanderRadius);
}

float UMFHomeAnchorComponent::GetDistanceFromHome() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !bHomeSet) return 0.f;

	// 2D 水平距离：俯视视角 + 坡面下不把高度差算进 leash。
	return FVector::Dist2D(Owner->GetActorLocation(), HomeLocation);
}

bool UMFHomeAnchorComponent::IsBeyondWander() const
{
	if (!IsEnabled()) return false;
	return GetDistanceFromHome() > Config.WanderRadius;
}

bool UMFHomeAnchorComponent::IsAtHome() const
{
	if (!bHomeSet) return true;   // 无家视为在家，不触发回家
	return GetDistanceFromHome() < Config.HomeArrivalTolerance;
}

void UMFHomeAnchorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if ENABLE_DRAW_DEBUG
	if (GHomeAnchorDebug && bHomeSet)
	{
		if (const UWorld* World = GetWorld())
		{
			// 锚点十字
			DrawDebugCrosshairs(World, HomeLocation, FRotator::ZeroRotator, 40.f, FColor::White, false, -1.f);
			// 游离半径绿圈（水平）
			DrawDebugCircle(World, HomeLocation, Config.WanderRadius, 32, FColor::Green,
				false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
			// owner → home 连线（离家越远越红）
			if (const AActor* Owner = GetOwner())
			{
				const float Ratio = Config.WanderRadius > 0.f
					? FMath::Clamp(GetDistanceFromHome() / Config.WanderRadius, 0.f, 1.f) : 0.f;
				const FColor LineColor = IsBeyondWander() ? FColor::Red : FColor::Yellow;
				DrawDebugLine(World, Owner->GetActorLocation(), HomeLocation, LineColor, false, -1.f, 0, 2.f);
			}
		}
	}
#endif
}

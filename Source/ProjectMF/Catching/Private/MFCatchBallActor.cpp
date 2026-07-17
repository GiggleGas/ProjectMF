// Copyright ProjectMF. All Rights Reserved.

#include "MFCatchBallActor.h"
#include "MFLog.h"

ACatchBallActor::ACatchBallActor()
{
	// 位置由外部 AT_MoveBall 驱动，不需自 Tick（基类已 bCanEverTick=false）。
	// 外观走基类 FlipbookComponent（BP 配球的 flipbook），billboard 由基类 UMFSpriteVisualComponent 接管。
}

void ACatchBallActor::BeginPlay()
{
	Super::BeginPlay();
	MF_LOG(LogMFCatch, TEXT("ACatchBallActor: Spawned at %s"),
		*GetActorLocation().ToString());
}

void ACatchBallActor::SetBallWorldLocation(const FVector& NewLocation)
{
	SetActorLocation(NewLocation);
}

FVector ACatchBallActor::GetBallWorldLocation() const
{
	return GetActorLocation();
}

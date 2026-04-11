// Copyright ProjectMF. All Rights Reserved.

#include "MFCatchBallActor.h"
#include "MFLog.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ACatchBallActor::ACatchBallActor()
{
	PrimaryActorTick.bCanEverTick = false; // 位置由外部 Task 驱动，不需要自 Tick

	// 碰撞根
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->SetSphereRadius(20.f);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 原型阶段不参与碰撞
	SetRootComponent(SphereCollision);

	// 网格（在 Blueprint 子类中指定 StaticMesh 资产）
	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	BallMesh->SetupAttachment(SphereCollision);
	BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

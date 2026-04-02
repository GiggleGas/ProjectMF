// Copyright ProjectMF. All Rights Reserved.

#include "MFCamera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AMFCamera::AMFCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	RootComponent = SpringArmComponent;
	SpringArmComponent->TargetArmLength = 800.0f;
	SpringArmComponent->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
}

void AMFCamera::BeginPlay()
{
	Super::BeginPlay();
}

void AMFCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FollowTarget)
	{
		const FVector TargetLocation = FollowTarget->GetActorLocation();
		const FVector CurrentLocation = GetActorLocation();
		const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, FollowSpeed);
		SetActorLocation(NewLocation);
	}
}

void AMFCamera::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFCamera.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

UMFCameraController::UMFCameraController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMFCameraController::Initialize(USpringArmComponent* InSpringArm, UCameraComponent* InCamera)
{
	SpringArm         = InSpringArm;
	Camera            = InCamera;
	TargetArmLength   = DefaultArmLength;
	SpriteOrientationYaw = 0.f;
	CurrentPositionIndex = 0;
}

void UMFCameraController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bZooming) return;

	USpringArmComponent* Arm = SpringArm.Get();
	if (!Arm) return;

	const float NewLength = FMath::FInterpTo(Arm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
	Arm->TargetArmLength = NewLength;

	if (FMath::IsNearlyEqual(NewLength, TargetArmLength, 1.0f))
	{
		Arm->TargetArmLength = TargetArmLength;
		bZooming = false;
	}
}

void UMFCameraController::SnapCamera(int32 Direction)
{
	// Wrap index in [0, 7]
	CurrentPositionIndex = (CurrentPositionIndex + Direction % 8 + 8) % 8;
	ApplySnapPosition();
}

void UMFCameraController::ApplySnapPosition()
{
	USpringArmComponent* Arm = SpringArm.Get();
	if (!Arm) return;

	const float NewYaw = CurrentPositionIndex * 45.f;

	FRotator Rot = Arm->GetRelativeRotation();
	Rot.Yaw = NewYaw;
	Arm->SetRelativeRotation(Rot);

	// Only update sprite orientation yaw at canonical 90° positions (even indices).
	// At 45° intermediate positions (odd indices) the sprite stays unchanged.
	if (CurrentPositionIndex % 2 == 0)
	{
		SpriteOrientationYaw = NewYaw;
	}
}

void UMFCameraController::ZoomTo(float NewTargetLength, float InterpSpeed)
{
	TargetArmLength  = FMath::Clamp(NewTargetLength, MinArmLength, MaxArmLength);
	ZoomInterpSpeed  = InterpSpeed;
	bZooming         = true;
}

void UMFCameraController::ResetZoom(float InterpSpeed)
{
	ZoomTo(DefaultArmLength, InterpSpeed);
}

void UMFCameraController::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass) return;

	const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
	if (!PC) return;

	PC->ClientStartCameraShake(ShakeClass, Scale);
}

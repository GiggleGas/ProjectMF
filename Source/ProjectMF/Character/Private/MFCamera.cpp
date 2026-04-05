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
	SpringArm            = InSpringArm;
	Camera               = InCamera;
	TargetArmLength      = DefaultArmLength;
	SpriteOrientationYaw = 0.f;
	CurrentPositionIndex = 0;
	CurrentYaw           = 0.f;
	TargetYaw            = 0.f;
	bRotating            = false;
}

void UMFCameraController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USpringArmComponent* Arm = SpringArm.Get();
	if (!Arm) return;

	// --- Smooth rotation ---
	if (bRotating)
	{
		if (RotationInterpSpeed <= 0.f)
		{
			// Speed = 0: instant snap
			CurrentYaw = TargetYaw;
			bRotating  = false;
		}
		else
		{
			CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, RotationInterpSpeed);
			if (FMath::IsNearlyEqual(CurrentYaw, TargetYaw, 0.1f))
			{
				CurrentYaw = TargetYaw;
				bRotating  = false;
			}
		}

		FRotator Rot = Arm->GetRelativeRotation();
		Rot.Yaw = CurrentYaw;
		Arm->SetRelativeRotation(Rot);
	}

	// --- Smooth zoom ---
	if (bZooming)
	{
		const float NewLength = FMath::FInterpTo(Arm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
		Arm->TargetArmLength = NewLength;

		if (FMath::IsNearlyEqual(NewLength, TargetArmLength, 1.0f))
		{
			Arm->TargetArmLength = TargetArmLength;
			bZooming = false;
		}
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
	// Compute the raw target yaw for this index.
	float NewYaw = CurrentPositionIndex * 45.f;

	// Unwind to take the shortest arc from the current interpolated position.
	while (NewYaw - CurrentYaw >  180.f) NewYaw -= 360.f;
	while (NewYaw - CurrentYaw < -180.f) NewYaw += 360.f;

	TargetYaw = NewYaw;
	bRotating = true;

	// Update sprite orientation immediately (responsive to input).
	// Only at canonical 90° positions (even indices); 45° intermediates keep the previous sprite.
	if (CurrentPositionIndex % 2 == 0)
	{
		// Normalise to [0, 360) for GetSpriteOrientationYaw callers.
		SpriteOrientationYaw = FMath::Fmod(TargetYaw + 360.f * 10.f, 360.f);
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

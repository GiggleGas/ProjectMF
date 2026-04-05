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
	SpringArm = InSpringArm;
	Camera = InCamera;
	TargetArmLength = DefaultArmLength;
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

void UMFCameraController::AddOrbitYaw(float Delta)
{
	USpringArmComponent* Arm = SpringArm.Get();
	if (!Arm) return;

	FRotator Rot = Arm->GetRelativeRotation();
	Rot.Yaw += Delta * OrbitYawSensitivity;
	Arm->SetRelativeRotation(Rot);
}

float UMFCameraController::GetOrbitYaw() const
{
	const USpringArmComponent* Arm = SpringArm.Get();
	return Arm ? Arm->GetRelativeRotation().Yaw : 0.f;
}

void UMFCameraController::ZoomTo(float NewTargetLength, float InterpSpeed)
{
	TargetArmLength = FMath::Clamp(NewTargetLength, MinArmLength, MaxArmLength);
	ZoomInterpSpeed = InterpSpeed;
	bZooming = true;
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

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFCamera.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCameraShakeBase;

/**
 * Camera controller component attached to MFCharacter.
 * Manages orbit rotation, zoom interpolation, and camera effects (shake, etc).
 *
 * Usage:
 *   1. CreateDefaultSubobject in character constructor, then call Initialize().
 *   2. Route input to AddOrbitYaw().
 *   3. Call ZoomTo() / PlayCameraShake() for effects.
 */
UCLASS(ClassGroup = "Camera", meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFCameraController : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFCameraController();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Link this controller to the character's spring arm and camera. Call from the owner's constructor. */
	void Initialize(USpringArmComponent* InSpringArm, UCameraComponent* InCamera);

	// --- Rotation ---

	/** Add yaw degrees to the orbit angle (driven by player input). */
	UFUNCTION(BlueprintCallable, Category = "Camera|Rotation")
	void AddOrbitYaw(float Delta);

	/** Return the current orbit yaw in world space. */
	UFUNCTION(BlueprintPure, Category = "Camera|Rotation")
	float GetOrbitYaw() const;

	// --- Zoom ---

	/** Smoothly interpolate the camera arm length to a new value. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Zoom")
	void ZoomTo(float NewTargetLength, float InterpSpeed = 5.0f);

	/** Reset zoom to DefaultArmLength. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Zoom")
	void ResetZoom(float InterpSpeed = 5.0f);

	// --- Effects ---

	/** Trigger a camera shake on the owning player controller. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Effects")
	void PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.0f);

protected:
	// --- Zoom config ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float DefaultArmLength = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinArmLength = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxArmLength = 2000.0f;

	// --- Rotation config ---

	/** Degrees of orbit added per unit of raw input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation")
	float OrbitYawSensitivity = 90.0f;

private:
	TWeakObjectPtr<USpringArmComponent> SpringArm;
	TWeakObjectPtr<UCameraComponent> Camera;

	float TargetArmLength = 1200.0f;
	float ZoomInterpSpeed = 5.0f;
	bool bZooming = false;
};

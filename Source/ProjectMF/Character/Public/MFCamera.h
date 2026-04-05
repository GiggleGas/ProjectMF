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
 *
 * Manages:
 *  - 8 discrete snap positions (every 45°) around the character
 *  - SpriteOrientationYaw: only updates at canonical 90° positions so that
 *    rotating to a 45° intermediate keeps the previous sprite unchanged
 *  - Zoom interpolation (ZoomTo / ResetZoom)
 *  - Camera shake (PlayCameraShake)
 *
 * Call Initialize() from the owning character's constructor after creating
 * the spring arm and camera components.
 */
UCLASS(ClassGroup = "Camera", meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFCameraController : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFCameraController();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Link this controller to the character's spring arm and camera. */
	void Initialize(USpringArmComponent* InSpringArm, UCameraComponent* InCamera);

	// --- Snap rotation ---

	/**
	 * Snap the camera to the next discrete position.
	 * Direction: +1 = clockwise (right), -1 = counter-clockwise (left).
	 * At 45° intermediate positions the SpriteOrientationYaw is NOT updated,
	 * preserving the sprite from the previous canonical (90°-multiple) position.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Rotation")
	void SnapCamera(int32 Direction);

	/** Current position index (0-7), each step is 45°. */
	UFUNCTION(BlueprintPure, Category = "Camera|Rotation")
	int32 GetCurrentPositionIndex() const { return CurrentPositionIndex; }

	/**
	 * Camera yaw used for sprite selection.
	 * Only updated when the camera is at a canonical 90° position (index 0,2,4,6).
	 * Returns a value in [0, 360).
	 */
	UFUNCTION(BlueprintPure, Category = "Camera|Rotation")
	float GetSpriteOrientationYaw() const { return SpriteOrientationYaw; }

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
	// --- Rotation config ---

	/**
	 * 相机旋转插值速度。值越大旋转越快；设为 0 则瞬间跳变（关闭平滑）。
	 * Interp speed for smooth camera rotation. Higher = faster; 0 = instant snap.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float RotationInterpSpeed = 10.f;

	// --- Zoom config ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float DefaultArmLength = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinArmLength = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxArmLength = 2000.0f;

private:
	TWeakObjectPtr<USpringArmComponent> SpringArm;
	TWeakObjectPtr<UCameraComponent>    Camera;

	// Current discrete position index (0–7). Maps to Yaw = Index * 45°.
	int32 CurrentPositionIndex = 0;

	// Yaw used for sprite selection; only valid at canonical positions (index even).
	float SpriteOrientationYaw = 0.0f;

	// Rotation smooth state
	float CurrentYaw = 0.f;   // actual spring arm yaw being interpolated
	float TargetYaw  = 0.f;   // destination yaw (unwound to take shortest path)
	bool  bRotating  = false;

	// Zoom state
	float TargetArmLength  = 1200.0f;
	float ZoomInterpSpeed  = 5.0f;
	bool  bZooming         = false;

	void ApplySnapPosition();
};

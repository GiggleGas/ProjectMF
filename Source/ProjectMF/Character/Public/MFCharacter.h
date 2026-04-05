// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MFCharacterTypes.h"
#include "MFCharacter.generated.h"

class UPaperFlipbookComponent;
class UPaperZDAnimationComponent;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UMFCameraController;
struct FInputActionValue;

UCLASS()
class PROJECTMF_API AMFCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMFCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Camera")
	UMFCameraController* GetCameraController() const { return CameraController; }

protected:
	virtual void BeginPlay() override;

	// --- Input Actions ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PickAction;

	/** 1D axis: +1 = snap CW (E key), -1 = snap CCW (Q key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RotateCameraAction;

	// --- Character State ---

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FMFCharacterState CharacterState;

	// --- Billboard ---

	/**
	 * Yaw offset added after aligning the sprite plane to the camera.
	 *   0   = sprite local +X faces camera
	 *  -90  = sprite local +Y faces camera  (typical for Paper2D XZ-plane sprites)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard")
	float BillboardYawOffset = 0.f;

	// --- Components ---
	/** Flipbook render component driven by PaperZD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	/** PaperZD animation component: owns the AnimInstance and drives FlipbookComponent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperZDAnimationComponent> AnimationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMFCameraController> CameraController;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandlePickStarted();
	void HandlePickCompleted();
	void HandleCameraRotate(const FInputActionValue& Value);

	void UpdateCharacterAction();
	void UpdateAnimation();
	void UpdateBillboard();

	EMFCameraRelativeDir GetCameraRelativeDir() const;
	static float         GetFacingYaw(EMFFacingDirection Dir);

};

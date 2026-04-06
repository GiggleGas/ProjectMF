// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MFCharacterTypes.h"
#include "MFCharacterBase.generated.h"

class UPaperFlipbookComponent;
class UPaperZDAnimationComponent;

/**
 * Abstract base class for all MF characters (player and AI).
 *
 * Owns the 2D rendering components (Flipbook + PaperZD) and the shared per-frame
 * logic: character action state machine, animation driving, and billboard alignment.
 *
 * Subclasses provide the camera information needed for billboard and directional
 * animation by overriding the two virtual accessors below.
 *
 * Mass integration stubs live in IMFMassControllable (AI/Public/MFMassInterface.h).
 * Only AMFAICharacter implements that interface; this base stays input-agnostic.
 */
UCLASS(Abstract)
class PROJECTMF_API AMFCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AMFCharacterBase();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// -----------------------------------------------------------------------
	// 2D Rendering Components
	// -----------------------------------------------------------------------

	/** Flipbook render component driven by PaperZD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	/** PaperZD animation component: owns the AnimInstance and drives FlipbookComponent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperZDAnimationComponent> AnimationComponent;

	// -----------------------------------------------------------------------
	// Character State
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FMFCharacterState CharacterState;

	// -----------------------------------------------------------------------
	// Billboard
	// -----------------------------------------------------------------------

	/**
	 * Yaw offset added after aligning the sprite plane to the camera.
	 *   0   = sprite local +X faces camera
	 *  -90  = sprite local +Y faces camera  (typical for Paper2D XZ-plane sprites)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard")
	float BillboardYawOffset = -90.f;

	// -----------------------------------------------------------------------
	// Camera abstraction — override in subclasses
	// -----------------------------------------------------------------------

	/**
	 * Return the camera's world-space forward vector (the direction the camera is looking).
	 * UpdateBillboard() negates this to get the "toward-camera" direction and applies it
	 * uniformly to all sprites — so every sprite in the scene shares the same tilt
	 * regardless of its world position (parallel / isometric-style billboard).
	 *
	 * Player: returns CameraComponent->GetForwardVector().
	 * AI:     returns PlayerCameraManager rotation vector.
	 */
	virtual bool GetBillboardCameraForward(FVector& OutForward) const { return false; }

	/**
	 * Return the yaw (degrees) of the in-game camera for directional animation mapping.
	 * This is fed into ComputeDirectionalInput() to produce camera-relative facing.
	 *
	 * Player: returns CameraController->GetSpriteOrientationYaw().
	 * AI:     returns the player camera manager yaw.
	 */
	virtual float GetCameraYawForDirectionality() const { return 0.f; }

	// -----------------------------------------------------------------------
	// Shared per-frame logic (called from Tick)
	// -----------------------------------------------------------------------

	/** Update CharacterState.CurrentAction based on velocity and bIsPicking. */
	void UpdateCharacterAction();

	/**
	 * Drive the AnimInstance (UMFAnimInstanceBase) with Speed, bIsPicking,
	 * and DirectionalInput. Override if you need character-specific anim logic.
	 */
	virtual void UpdateAnimation();

	/** Rotate FlipbookComponent to always face the billboard camera. */
	void UpdateBillboard();

	/** Console-var-gated debug visualization (arrows + screen text). */
	virtual void DrawDebug() const;

	/**
	 * Compute a 2D camera-relative facing vector for PaperZD SetDirectionality.
	 * Uses CharacterState.LastVelocityDir and GetCameraYawForDirectionality().
	 */
	FVector2D GetDirectionalInput() const;
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFCharacterTypes.generated.h"

class UPaperFlipbook;
class UPaperZDAnimSequence;

// ============================================================
// Enums
// ============================================================

/** World-space facing direction, derived from movement velocity. */
UENUM(BlueprintType)
enum class EMFFacingDirection : uint8
{
	Right UMETA(DisplayName = "Right"),
	Left  UMETA(DisplayName = "Left"),
	Up    UMETA(DisplayName = "Up"),
	Down  UMETA(DisplayName = "Down"),
};

UENUM(BlueprintType)
enum class EMFCharacterAction : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk"),
	Pick UMETA(DisplayName = "Pick"),
};

/**
 * Camera-relative sprite direction.
 * Determined by the angle between world-facing direction and the camera's
 * orientation yaw (only updated at canonical 90° camera positions).
 */
UENUM(BlueprintType)
enum class EMFCameraRelativeDir : uint8
{
	Front UMETA(DisplayName = "Front"),
	Back  UMETA(DisplayName = "Back"),
	Left  UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
};

// ============================================================
// Structs
// ============================================================

USTRUCT(BlueprintType)
struct FMFCharacterState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EMFFacingDirection FacingDirection = EMFFacingDirection::Down;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EMFCharacterAction CurrentAction = EMFCharacterAction::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsPicking = false;
};

/** Legacy: world-axis sprite set used by UMFAnimationConfig. */
USTRUCT(BlueprintType)
struct FMFDirectionalFlipbooks
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> Right = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> Left = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> Up = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> Down = nullptr;
};

/** Camera-relative animation set: one PaperZD sequence per visible side. */
USTRUCT(BlueprintType)
struct FMFCameraRelativeSprites
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPaperZDAnimSequence> Front = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPaperZDAnimSequence> Back = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPaperZDAnimSequence> Left = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPaperZDAnimSequence> Right = nullptr;
};

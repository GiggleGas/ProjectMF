// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFCharacterTypes.generated.h"


// ============================================================
// Enums
// ============================================================

UENUM(BlueprintType)
enum class EMFCharacterAction : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk"),
	Pick UMETA(DisplayName = "Pick"),
};

// ============================================================
// Structs
// ============================================================

USTRUCT(BlueprintType)
struct FMFCharacterState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EMFCharacterAction CurrentAction = EMFCharacterAction::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsPicking = false;

	/** Last non-zero movement direction in world XY (normalized). Kept when idle so the sprite holds its last facing. */
	FVector2D LastVelocityDir = FVector2D(1.f, 0.f);
};


// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperZDAnimInstance.h"
#include "MFAnimInstanceBase.generated.h"

/**
 * Shared AnimInstance base for all MF characters (player and AI).
 * Holds the three properties that PaperZD AnimBPs read each frame.
 * Derive from this for each character type so each can have its own AnimBP asset.
 */
UCLASS(Abstract)
class PROJECTMF_API UMFAnimInstanceBase : public UPaperZDAnimInstance
{
	GENERATED_BODY()

public:
	/** Ground speed (XY plane). Used to drive walk/idle transitions. */
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float Speed = 0.f;

	/** True while the character is in a picking/interacting action. */
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsPicking = false;

	/**
	 * Camera-relative facing direction for the PaperZD SetDirectionality node.
	 * Expressed as a 2D unit vector in camera space:
	 *   (0, 1)  = 0°   moving away from camera (back sprite)
	 *   (1, 0)  = 90°  moving camera-right
	 *   (0,-1)  = 180° moving toward camera (front sprite)
	 *  (-1, 0)  = -90° moving camera-left
	 * Wire this directly to SetDirectionality.Input in the AnimBP.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	FVector2D DirectionalInput = FVector2D(0.f, 1.f);
};

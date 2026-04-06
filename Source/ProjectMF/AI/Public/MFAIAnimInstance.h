// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAnimInstanceBase.h"
#include "MFAIAnimInstance.generated.h"

/**
 * AnimInstance for AI characters.
 * Inherits Speed, bIsPicking, and DirectionalInput from UMFAnimInstanceBase.
 *
 * Exists as a distinct class so the AI AnimBP asset references this type
 * independently from the player AnimBP (UMFPlayerAnimInstance).
 *
 * Add AI-exclusive anim properties here (e.g., AlertLevel, CombatStance) as needed.
 */
UCLASS()
class PROJECTMF_API UMFAIAnimInstance : public UMFAnimInstanceBase
{
	GENERATED_BODY()
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAnimInstanceBase.h"
#include "MFPlayerAnimInstance.generated.h"

/**
 * AnimInstance for the player character.
 * Driven each frame by AMFCharacterBase::UpdateAnimation().
 *
 * Inherits Speed, bIsPicking, and DirectionalInput from UMFAnimInstanceBase.
 * Exists as a distinct class so the player AnimBP asset references this type
 * independently from the AI AnimBP.
 *
 * Add player-exclusive anim properties here if needed in the future.
 */
UCLASS()
class PROJECTMF_API UMFPlayerAnimInstance : public UMFAnimInstanceBase
{
	GENERATED_BODY()
};

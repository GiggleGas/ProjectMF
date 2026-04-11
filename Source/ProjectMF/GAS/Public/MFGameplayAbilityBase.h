// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MFGameplayAbilityBase.generated.h"

class AMFCharacterBase;

/**
 * Abstract base class for all ProjectMF GameplayAbilities.
 *
 * Provides a typed convenience accessor for the owning character so that
 * derived abilities (GA_Pick, GA_Attack, pet skills, etc.) don't each need
 * to cast the avatar actor themselves.
 *
 * Blueprint usage:
 *   - Create Blueprint children of this class for abilities that need BP logic.
 *   - Override ActivateAbility (and optionally EndAbility) in the child BP.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API UMFGameplayAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * Returns the Avatar actor cast to AMFCharacterBase.
	 * Returns nullptr if the avatar is not an MF character (should not happen in normal play).
	 */
	UFUNCTION(BlueprintPure, Category = "Ability|MF")
	AMFCharacterBase* GetMFCharacter() const;
};

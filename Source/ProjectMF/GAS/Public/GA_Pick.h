// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "GA_Pick.generated.h"

/**
 * Pick / gather GameplayAbility.
 *
 * Lifecycle:
 *   - Activated by AMFCharacter::HandlePickStarted via TryActivateAbilitiesByTag.
 *   - Cancelled by AMFCharacter::HandlePickCompleted via CancelAbilitiesByTag.
 *   - AI characters receive this via FMFAICommand.AbilityTagToActivate.
 *
 * Tag contract:
 *   AbilityTags          = { MF.Ability.Pick }      — used to find/activate/cancel this ability.
 *   ActivationOwnedTags  = { MF.Character.State.Picking } — automatically added to the ASC
 *                          owner while active, removed when ended or cancelled.
 *
 * Animation integration:
 *   AMFCharacterBase::UpdateCharacterAction() polls the ASC for MF.Character.State.Picking
 *   each tick and writes CharacterState.bIsPicking accordingly.
 *   This keeps the existing PaperZD animation pipeline fully intact with no extra changes.
 */
UCLASS()
class PROJECTMF_API UGA_Pick : public UMFGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_Pick();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle    Handle,
		const FGameplayAbilityActorInfo*    ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*           TriggerEventData) override;
};

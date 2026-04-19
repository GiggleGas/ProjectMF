// Copyright ProjectMF. All Rights Reserved.

#include "GA_Pick.h"
#include "MFGameplayTags.h"

UGA_Pick::UGA_Pick()
{
	// This ability is found and activated/cancelled by tag.
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pick));

	// MF.Character.State.Picking is automatically added to the owning ASC while this
	// ability is active and removed the moment it ends or is cancelled.
	// AMFCharacterBase::UpdateCharacterAction() reads this tag — no manual bIsPicking
	// assignment needed in this class.
	ActivationOwnedTags.AddTag(MFGameplayTags::State_Picking);

	// Only one Pick ability runs at a time per actor.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Pick::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	// CommitAbility checks and applies cost/cooldown.
	// If it fails (e.g. not enough resources), cancel immediately.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ActivationOwnedTags handles the rest: MF.Character.State.Picking is now live on
	// the ASC, and UpdateCharacterAction() will pick it up next tick.
	// The ability stays active until CancelAbilitiesByTag is called from the input handler.
}

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Centralized native GameplayTag declarations for ProjectMF.
 *
 * Use these constants everywhere instead of raw FName/FString literals to get
 * compile-time safety and a single place to rename tags.
 *
 * Tag naming convention:
 *   MF.<Domain>.<SubDomain>.<Name>
 *   e.g.  MF.Character.State.Picking
 *         MF.Ability.Pick
 */
namespace MFGameplayTags
{
	// -----------------------------------------------------------------------
	// Character State Tags
	// Automatically owned by the ASC while the corresponding ability is active.
	// AMFCharacterBase::UpdateCharacterAction() reads these to update CharacterState.
	// -----------------------------------------------------------------------

	/** Owned while the Pick/gather ability is active.
	 *  Drives EMFCharacterAction::Pick and the bIsPicking animation flag. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Picking);

	// -----------------------------------------------------------------------
	// Ability Tags
	// Used with TryActivateAbilitiesByTag / CancelAbilitiesByTag.
	// -----------------------------------------------------------------------

	/** Identifies the Pick/gather GameplayAbility. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pick);

	/** Identifies the CatchPet GameplayAbility. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_CatchPet);

	// -----------------------------------------------------------------------
	// Catching State Tags
	// Owned by the ASC to track the active phase of the catch-pet ability.
	// -----------------------------------------------------------------------

	/** Player is in aim mode: preview line drawn, hovering for a target. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Catching_State_Aiming);

	/** Rope thrown and connected; ball bouncing between player and pet. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Catching_State_RopeActive);

	/** Ball has reached the player — waiting for Space bar input (QTE). */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Catching_State_WaitingForBounce);

	// -----------------------------------------------------------------------
	// Catching Event Tags
	// Used with SendGameplayEvent to communicate across tasks/abilities.
	// -----------------------------------------------------------------------

	/** Fired when the catch attempt succeeds (all bounces complete). */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Catching_Event_Success);

	/** Fired when the catch attempt fails (QTE timeout or other failure). */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Catching_Event_Fail);
}

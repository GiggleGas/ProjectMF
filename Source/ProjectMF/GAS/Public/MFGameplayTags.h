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

	/**
	 * Granted when Health reaches 0 (via HandleDeath).
	 * Blocks further ability activation. StateTree and UI listen for this tag.
	 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	/**
	 * Granted when a character enters combat (has an active enemy target).
	 * Used by StateTree to switch between Follow and Combat states.
	 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InCombat);

	// -----------------------------------------------------------------------
	// Ability Tags
	// Used with TryActivateAbilitiesByTag / CancelAbilitiesByTag.
	// -----------------------------------------------------------------------

	/** Identifies the Pick/gather GameplayAbility. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pick);

	/** Identifies the CatchPet GameplayAbility. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_CatchPet);

	/**
	 * Identifies any AI attack GameplayAbility.
	 * STTask_ActivateAttack finds and activates the first granted ability with this tag.
	 * Assign this tag in AbilityTags on the attack ability blueprint.
	 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);

	/**
	 * GameplayEvent tag for triggering GA_SummonPet.
	 * EventMagnitude carries the pet slot index (1-5).
	 * Sources: demo key bindings (1-5), future: GA_PetWheel confirmation.
	 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_SummonPet);

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

	// -----------------------------------------------------------------------
	// Team Tags
	// Placed on character ASCs to identify faction for attack filtering.
	// FilterTarget() in UGA_AIAttackBase compares these between caster and target.
	// -----------------------------------------------------------------------

	/** Owned by player-faction characters. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Player);

	/** Owned by enemy/AI-faction characters. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Enemy);

	// -----------------------------------------------------------------------
	// Attack Data Tags
	// Used as SetByCaller keys in damage GameplayEffect specs.
	// -----------------------------------------------------------------------

	/**
	 * SetByCaller magnitude key for attack damage.
	 * The damage GE must read this tag via GetSetByCallerMagnitude.
	 * Written by UGA_AIAttackBase::ApplyDamageToTarget with DamageMultiplier.
	 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Data_Damage);

	// -----------------------------------------------------------------------
	// Attack State Tags
	// -----------------------------------------------------------------------

	/** Granted while an AI attack ability is active. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);
}

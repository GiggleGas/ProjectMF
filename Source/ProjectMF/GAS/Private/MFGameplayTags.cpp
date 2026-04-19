// Copyright ProjectMF. All Rights Reserved.

#include "MFGameplayTags.h"

namespace MFGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Picking,
		"MF.Character.State.Picking",
		"Owned by the ASC while the Pick ability is active. Drives the Pick animation state.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Dead,
		"MF.Character.State.Dead",
		"Granted when Health reaches 0. Blocks ability activation; monitored by StateTree and UI.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_InCombat,
		"MF.Character.State.InCombat",
		"Granted when a character has an active combat target. Drives StateTree Combat state.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Pick,
		"MF.Ability.Pick",
		"Ability tag for the Pick/gather ability. Used with TryActivateAbilitiesByTag.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_CatchPet,
		"MF.Ability.CatchPet",
		"Ability tag for the CatchPet ability. Activated on catch-key release.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Attack,
		"MF.Ability.Attack",
		"Identifies any AI attack ability. STTask_ActivateAttack finds the first granted ability with this tag.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_SummonPet,
		"MF.Ability.SummonPet",
		"GameplayEvent tag for GA_SummonPet. EventMagnitude = slot index (1-5). "
		"Fired by demo key bindings now, by GA_PetWheel in the future.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Catching_State_Aiming,
		"MF.Catching.State.Aiming",
		"Player is in aiming phase: preview line shown, hovering to select target.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Catching_State_RopeActive,
		"MF.Catching.State.RopeActive",
		"Rope thrown and connected; ball bouncing between player and pet.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Catching_State_WaitingForBounce,
		"MF.Catching.State.WaitingForBounce",
		"Ball reached the player side — waiting for Space bar QTE input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Catching_Event_Success,
		"MF.Catching.Event.Success",
		"Sent when all bounces complete and the pet is successfully caught.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Catching_Event_Fail,
		"MF.Catching.Event.Fail",
		"Sent when the catch attempt fails (QTE timeout or other failure).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Team_Player,
		"MF.Team.Player",
		"Owned by player-faction characters. Used by UGA_AIAttackBase::FilterTarget for enemy/ally detection.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Team_Enemy,
		"MF.Team.Enemy",
		"Owned by enemy/AI-faction characters. Used by UGA_AIAttackBase::FilterTarget for enemy/ally detection.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Attack_Data_Damage,
		"MF.Attack.Data.Damage",
		"SetByCaller key written into damage GE specs by UGA_AIAttackBase. "
		"The damage GameplayEffect must read this via GetSetByCallerMagnitude.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Attacking,
		"MF.Character.State.Attacking",
		"Granted to the ASC while an AI attack ability is active.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		AI_Perception_HasTarget,
		"MF.AI.Perception.HasTarget",
		"Indicates that the AI's radar has at least one valid perceived target. "
		"Managed by the threat system; queried by StateTree to decide combat transitions.");
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFGameplayTags.h"

namespace MFGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Picking,
		"MF.Character.State.Picking",
		"Owned by the ASC while the Pick ability is active. Drives the Pick animation state.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Pick,
		"MF.Ability.Pick",
		"Ability tag for the Pick/gather ability. Used with TryActivateAbilitiesByTag.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_CatchPet,
		"MF.Ability.CatchPet",
		"Ability tag for the CatchPet ability. Activated on catch-key release.");

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
}

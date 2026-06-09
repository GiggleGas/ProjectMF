// Copyright ProjectMF. All Rights Reserved.

#include "MFGameplayTags.h"

namespace MFGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Picking,
		"MF.GameplayState.Picking",
		"Owned by the ASC while the Pick ability is active. Drives the Pick animation state.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Dead,
		"MF.GameplayState.Dead",
		"Granted when Health reaches 0. Blocks ability activation; monitored by StateTree and UI.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_InCombat,
		"MF.GameplayState.InCombat",
		"Granted when a character has an active combat target. Drives StateTree Combat state.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Stunned,
		"MF.GameplayState.Stunned",
		"Stunned: blocks ability activation (A3) and movement (B7). Granted by GE_Stun / GE_Freeze.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Slowed,
		"MF.GameplayState.Slowed",
		"Slowed: for VFX / anim / queries (the slow magnitude is applied via MoveSpeed). Granted by GE_Slow.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Blinded,
		"MF.GameplayState.Blinded",
		"Blinded: target acquisition disabled (B8). Granted by GE_Blind.");

	// ----- 玩家技能 -----
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player, "MF.Ability.Player",
		"Category: all player-cast abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Pick, "MF.Ability.Player.Pick",
		"Pick/gather ability (GA_Pick). Set in C++ ctor; activated via TryActivateAbilitiesByTag.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_CatchPet, "MF.Ability.Player.CatchPet",
		"CatchPet ability (GA_CatchPet). Activated on catch-key release.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_SummonPet, "MF.Ability.Player.SummonPet",
		"GameplayEvent tag for GA_SummonPet. EventMagnitude = slot index (1-5).");

	// ----- 宠物 / AI 战斗技能（宠物 + 敌人 + Boss）-----
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet, "MF.Ability.Pet",
		"Category: all AI-combatant abilities (pets / enemies / boss).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Melee, "MF.Ability.Pet.Melee",
		"Melee / AOE attack (UGA_AIAttackBase). STTask_ActivateAttack finds the granted ability by this tag.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Ranged, "MF.Ability.Pet.Ranged",
		"Category: all ranged attacks. Use specific child tags in StateTree.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Ranged_Throw, "MF.Ability.Pet.Ranged.Throw",
		"Throw-projectile ranged attack (UGA_ThrowProjectile).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Ranged_Boulder, "MF.Ability.Pet.Ranged.Boulder",
		"Falling-boulder ranged attack (UGA_FallingBoulder).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Ranged_BulletCurtain, "MF.Ability.Pet.Ranged.BulletCurtain",
		"Bullet-curtain ranged attack (UGA_BulletCurtain).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Move, "MF.Ability.Pet.Move",
		"Category: movement abilities (P5).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Move_Charge, "MF.Ability.Pet.Move.Charge",
		"Charge/dash ability (P5, not yet implemented).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Move_Jump, "MF.Ability.Pet.Move.Jump",
		"Jump ability (P5, not yet implemented).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Pet_Move_GroundSlam, "MF.Ability.Pet.Move.GroundSlam",
		"Ground-slam ability (P5, not yet implemented).");

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
		Team,
		"MF.Team",
		"Parent category for faction tags. Enumerated by UMFFactionStatics to read a character's team membership.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Team_Player,
		"MF.Team.Player",
		"Owned by player-faction characters. Used by UMFFactionStatics::AreSameTeam for damage filtering.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Team_Enemy,
		"MF.Team.Enemy",
		"Owned by enemy/AI-faction characters. Used by UMFFactionStatics::AreSameTeam for damage filtering.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Team_Boss,
		"MF.Team.Boss",
		"Owned by Boss-faction characters. Separate faction from Team_Enemy; assigned at spawn by M1_SpawnBoss.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Attack_Data_Damage,
		"MF.Attack.Data.Damage",
		"SetByCaller key written into damage GE specs by UGA_AIAttackBase. "
		"The damage GameplayEffect must read this via GetSetByCallerMagnitude.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Attacking,
		"MF.GameplayState.Attacking",
		"Granted to the ASC while an AI attack ability is active.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		AI_Perception_HasTarget,
		"MF.AI.Perception.HasTarget",
		"Indicates that the AI's radar has at least one valid perceived target. "
		"Managed by the threat system; queried by StateTree to decide combat transitions.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_RangedAttacking,
		"MF.GameplayState.RangedAttacking",
		"Granted to the ASC while any ranged attack ability is active. "
		"Monitored as ActiveStateTag in STTask for ranged attacks.");

	// ----- Effect 身份标签（MF.Effect.*）-----
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect, "MF.Effect",
		"Parent category for all MF gameplay effects (combo / area identity).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Burn, "MF.Effect.Burn",
		"Burn: periodic damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Freeze, "MF.Effect.Freeze",
		"Freeze: behaves like stun (grants State.Stunned); distinguished visually via Cue.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Root, "MF.Effect.Root",
		"Root: MoveSpeed x0 (cannot move, can still cast). No State tag needed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Slow, "MF.Effect.Slow",
		"Slow: reduces MoveSpeed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Stun, "MF.Effect.Stun",
		"Stun: grants State.Stunned (blocks abilities + movement).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Vulnerable, "MF.Effect.Vulnerable",
		"Vulnerable: raises IncomingDamageMultiplier.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_DamageUp, "MF.Effect.DamageUp",
		"Damage-up: raises OutgoingDamageMultiplier.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Heal, "MF.Effect.Heal",
		"Heal: writes the Healing meta attribute.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Blind, "MF.Effect.Blind",
		"Blind: grants State.Blinded (target acquisition disabled).");
}

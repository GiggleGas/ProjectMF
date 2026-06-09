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
	// Ability Tags（按拥有者 / 功能分层：MF.Ability.<Player|Pet>.<...>）
	// 由各 GA 的 C++ 构造函数 SetAssetTags 设置，不在 BP 配置。
	// 用于 TryActivateAbilitiesByTag / GetAssetTags().HasTag / StateTree 选取。
	// 层次匹配：HasTag(Ability.Pet) 命中所有宠物技能，HasTag(Ability.Pet.Ranged) 命中所有远程。
	// -----------------------------------------------------------------------

	// --- 玩家技能 ---
	/** 类别：所有玩家释放的技能。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player);
	/** 采集 / 拾取（GA_Pick）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Pick);
	/** 抓宠（GA_CatchPet）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_CatchPet);
	/** 召唤 / 召回宠物（GA_SummonPet，GameplayEvent，EventMagnitude = slot 1-5）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_SummonPet);

	// --- 宠物 / AI 战斗技能（涵盖宠物 + 敌人 + Boss）---
	/** 类别：所有 AI 战斗者释放的技能。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet);
	/** 近战 / AOE（GA_AIAttackBase）。STTask_ActivateAttack 按此 tag 找已授予技能。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Melee);

	/** 类别：所有远程攻击（GA_AIRangedAttackBase）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Ranged);
	/** 直线投掷（GA_ThrowProjectile）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Ranged_Throw);
	/** 天降落石（GA_FallingBoulder）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Ranged_Boulder);
	/** 弹幕（GA_BulletCurtain）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Ranged_BulletCurtain);

	/** 类别：移动技能（P5）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Move);
	/** 冲撞（P5，未实现）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Move_Charge);
	/** 跳跃（P5，未实现）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Move_Jump);
	/** 撼地（P5，未实现）。 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pet_Move_GroundSlam);

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

	/** Parent category for all faction tags (MF.Team.*). Used to enumerate a character's team membership. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team);

	/** Owned by player-faction characters. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Player);

	/** Owned by enemy/AI-faction characters. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Enemy);

	/** Owned by Boss-faction characters. Separate from Team_Enemy so summoned pets and Boss are distinct factions. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Team_Boss);

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

	// -----------------------------------------------------------------------
	// AI Perception Tags
	// 用于感知系统与 StateTree / 威胁系统通信。
	// -----------------------------------------------------------------------

	/**
	 * AI 雷达感知到至少一个目标时，由外部系统（如威胁管理器）授予。
	 * StateTree 可用此标签作为条件判断：是否有感知目标存在。
	 * 注：RadarSensingComponent 本身不自动授予此 Tag，需要威胁系统来管理。
	 */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Perception_HasTarget);

	// -----------------------------------------------------------------------
	// Ranged Attack State Tag
	// （Ability.Pet.Ranged.* 已上移到 Ability Tags 段）
	// -----------------------------------------------------------------------

	/** Granted to the ASC while any ranged attack ability is active. */
	PROJECTMF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_RangedAttacking);
}

// Copyright ProjectMF. All Rights Reserved.

#include "GA_GroundSlam.h"
#include "MFGameplayTags.h"

UGA_GroundSlam::UGA_GroundSlam()
{
	// 独立 Ability tag = 激活/冷却身份，与基础近战(Ability_Pet_Melee)区分。
	// 注：命名空间仍沿用 Move（历史标签），后续按计划纠正为非 Move（改常量 + redirect）。
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pet_Move_GroundSlam));
}

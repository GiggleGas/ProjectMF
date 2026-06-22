// Copyright ProjectMF. All Rights Reserved.

#include "STCond_CanAutoUseSkill.h"

#include "MFGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTCond_CanAutoUseSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FSTCond_CanAutoUseSkill::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (!InstanceData.AbilityTag.IsValid())
	{
		return false;
	}

	const AAIController& Controller = Context.GetExternalData(AIControllerHandle);
	APawn* Pawn = Controller.GetPawn();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC)
	{
		return false;
	}

	// 1. 找 AbilityTag 对应的已授予技能。
	const FGameplayAbilitySpec* Found = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(InstanceData.AbilityTag))
		{
			Found = &Spec;
			break;
		}
	}
	if (!Found || !Found->Ability)
	{
		return false;
	}

	// 2. 冷却中 → 不可（冷却身份标签 = 技能自身 AbilityTag，由 ApplyCooldown 动态授予）。
	if (const FGameplayTagContainer* CooldownTags = Found->Ability->GetCooldownTags())
	{
		if (ASC->HasAnyMatchingGameplayTags(*CooldownTags))
		{
			return false;
		}
	}

	// 3. 非召唤（敌人/野宠）→ 总是允许自动；召唤宠物 → 仅当 spec 标了 Auto。
	if (!ASC->HasMatchingGameplayTag(MFGameplayTags::Pet_Summoned))
	{
		return true;
	}
	return Found->GetDynamicSpecSourceTags().HasTag(MFGameplayTags::SkillMode_Auto);
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFCooldownGameplayEffect.h"
#include "MFGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UMFCooldownGameplayEffect::UMFCooldownGameplayEffect()
{
	// 有时长，时长由调用方通过 SetByCaller(MF.Data.Cooldown) 提供。
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSBC;
	DurationSBC.DataTag = MFGameplayTags::Data_Cooldown;
	DurationMagnitude   = FGameplayEffectModifierMagnitude(DurationSBC);

	// 静态授予占位标签，满足 UE 蓝图校验器（"GE must grant tags to be used as cooldown"）。
	// UE5.3+ 改用 GEComponent 管理授予标签；InheritableGrantedTagsContainer 已移入
	// UTargetTagsGameplayEffectComponent，直接访问会编译失败。
	// 实际冷却身份（技能 AbilityTag）仍由 ApplyCooldown 运行时写入 DynamicGrantedTags。
	UTargetTagsGameplayEffectComponent& TagsComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(MFGameplayTags::Cooldown_Ability);
	TagsComp.SetAndApplyTargetTagChanges(TagChanges);
}

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
	// UE5.3+ 授予标签由 UTargetTagsGameplayEffectComponent 管理（旧 InheritableGrantedTagsContainer 已移入它）。
	//
	// ⚠️ 必须用 CreateDefaultSubobject 建组件：FindOrAddComponent 内部走 NewObject，
	//    在 CDO 构造期(Default__MFCooldownGameplayEffect)调用会 crash
	//    （引擎只在 PostLoad/PostInitProperties 等构造后时机用 FindOrAddComponent）。
	//    CreateDefaultSubobject 是构造期安全原语，且 GEComponents 是 Instanced 数组，正合此法。
	//
	// 实际冷却身份（技能 AbilityTag）仍由 ApplyCooldown 运行时写入 DynamicGrantedTags。
	UTargetTagsGameplayEffectComponent* TagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("CooldownGrantedTags"));
	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(MFGameplayTags::Cooldown_Ability);
	TagsComp->SetAndApplyTargetTagChanges(TagChanges);
	GEComponents.Add(TagsComp);
}

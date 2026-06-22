// Copyright ProjectMF. All Rights Reserved.

#include "MFPetGameplayAbility.h"
#include "MFCombatStatics.h"
#include "MFAttackDataBase.h"
#include "MFCooldownGameplayEffect.h"
#include "MFGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"

UMFPetGameplayAbility::UMFPetGameplayAbility()
{
	// 所有 AI 技能共用同一个 C++ 冷却 GE；时长读自数据资产 CooldownSeconds。
	CooldownGameplayEffectClass = UMFCooldownGameplayEffect::StaticClass();
}

const FGameplayTagContainer* UMFPetGameplayAbility::GetCooldownTags() const
{
	// 用技能自身唯一的 AbilityTag(s) 作为冷却身份（ApplyCooldown 会动态授予这些标签）。
	return &GetAssetTags();
}

void UMFPetGameplayAbility::ApplyCooldown(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UMFAttackDataBase* Data = GetAttackDataBase();
	const float Cooldown = Data ? Data->CooldownSeconds : 0.f;
	if (Cooldown <= 0.f)
	{
		return; // 无冷却
	}

	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MFGameplayTags::Data_Cooldown, Cooldown);
		// 动态授予技能自身 AbilityTag 作为冷却身份；与 GetCooldownTags 对应。
		Spec.Data->DynamicGrantedTags.AppendTags(GetAssetTags());
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

float UMFPetGameplayAbility::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo) return 0.f;
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return 0.f;

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(GetAssetTags());
	float MaxRemaining = 0.f;
	for (float Remaining : ASC->GetActiveEffectsTimeRemaining(Query))
	{
		MaxRemaining = FMath::Max(MaxRemaining, Remaining);
	}
	return MaxRemaining;
}

void UMFPetGameplayAbility::ApplyOnHitEffects(AActor* Target, const TArray<FMFOnHitEffect>& Effects)
{
	if (!Target) return;

	// 薄包装：解析来源/目标 ASC 后转交共享静态（与区域子系统同一套施加逻辑）。
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UMFCombatStatics::ApplyOnHitEffects(SourceASC, TargetASC, Effects, GetAbilityLevel());
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFPetGameplayAbility.h"
#include "MFAttackTypes.h"
#include "MFCombatEffectSettings.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"

void UMFPetGameplayAbility::ApplyOnHitEffects(AActor* Target, const TArray<FMFOnHitEffect>& Effects)
{
	if (!Target || Effects.Num() == 0) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC) return;

	const UMFCombatEffectSettings* Settings = GetDefault<UMFCombatEffectSettings>();
	if (!Settings) return;

	for (const FMFOnHitEffect& OnHit : Effects)
	{
		// 概率判定（每个目标、每次命中独立 roll）。
		if (OnHit.Chance < 1.f && FMath::FRand() >= OnHit.Chance) continue;

		// 按 Kind 从映射表解析 GE 与「数值写哪些 SetByCaller 键」。
		const FMFEffectKindDef* Def = Settings->EffectMap.Find(OnHit.Kind);
		if (!Def || !Def->Effect) continue;

		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Def->Effect, GetAbilityLevel());
		if (!Spec.IsValid()) continue;

		if (Def->DurationTag.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(Def->DurationTag, OnHit.Duration);
		}
		if (Def->MagnitudeTag.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(Def->MagnitudeTag, OnHit.Magnitude);
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

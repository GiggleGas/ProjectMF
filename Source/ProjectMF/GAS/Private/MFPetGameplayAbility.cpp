// Copyright ProjectMF. All Rights Reserved.

#include "MFPetGameplayAbility.h"
#include "MFAttackTypes.h"
#include "MFGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"

void UMFPetGameplayAbility::ApplyOnHitEffects(AActor* Target, const TArray<FMFOnHitEffect>& Effects)
{
	if (!Target || Effects.Num() == 0) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC) return;

	for (const FMFOnHitEffect& OnHit : Effects)
	{
		if (!OnHit.Effect) continue;

		// 概率判定（每个目标、每次命中独立 roll）。
		if (OnHit.Chance < 1.f && FMath::FRand() >= OnHit.Chance) continue;

		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(OnHit.Effect, GetAbilityLevel());
		if (!Spec.IsValid()) continue;

		// 把时长通过 SetByCaller 写入，使同一个 GE 资源支持不同持续时间。
		Spec.Data->SetSetByCallerMagnitude(MFGameplayTags::Data_Duration, OnHit.Duration);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFPetGameplayAbility.h"
#include "MFCombatStatics.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

void UMFPetGameplayAbility::ApplyOnHitEffects(AActor* Target, const TArray<FMFOnHitEffect>& Effects)
{
	if (!Target) return;

	// 薄包装：解析来源/目标 ASC 后转交共享静态（与区域子系统同一套施加逻辑）。
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UMFCombatStatics::ApplyOnHitEffects(SourceASC, TargetASC, Effects, GetAbilityLevel());
}

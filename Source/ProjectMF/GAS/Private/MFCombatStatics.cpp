// Copyright ProjectMF. All Rights Reserved.

#include "MFCombatStatics.h"
#include "MFCombatEffectSettings.h"
#include "MFCombatAttributeSet.h"
#include "MFGameplayTags.h"
#include "MFAreaEffectData.h"
#include "MFAreaEffectSubsystem.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"

void UMFCombatStatics::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	TSubclassOf<UGameplayEffect> DamageGE, float DamageMultiplier)
{
	if (!Source || !Target || !DamageGE)
	{
		return;
	}

	float AttackValue  = 0.f;
	float OutgoingMult = 1.f;
	if (const UMFCombatAttributeSet* CombatSet = Source->GetSet<UMFCombatAttributeSet>())
	{
		AttackValue  = CombatSet->GetAttack();
		OutgoingMult = CombatSet->GetOutgoingDamageMultiplier();
	}

	const float FinalMagnitude = AttackValue * DamageMultiplier * OutgoingMult;

	FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(DamageGE, 1.f, Source->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(MFGameplayTags::Attack_Data_Damage, FinalMagnitude);
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
}

void UMFCombatStatics::ApplyOnHitEffects(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	const TArray<FMFOnHitEffect>& Effects, float Level)
{
	if (!Source || !Target || Effects.Num() == 0)
	{
		return;
	}

	const UMFCombatEffectSettings* Settings = GetDefault<UMFCombatEffectSettings>();
	if (!Settings)
	{
		return;
	}

	for (const FMFOnHitEffect& OnHit : Effects)
	{
		// 概率（每目标每次独立 roll）
		if (OnHit.Chance < 1.f && FMath::FRand() >= OnHit.Chance)
		{
			continue;
		}

		// 按 Kind 从映射表解析 GE 与「数值写哪些 SetByCaller 键」
		const FMFEffectKindDef* Def = Settings->EffectMap.Find(OnHit.Kind);
		if (!Def || !Def->Effect)
		{
			continue;
		}

		FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(Def->Effect, Level, Source->MakeEffectContext());
		if (!Spec.IsValid())
		{
			continue;
		}
		if (Def->DurationTag.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(Def->DurationTag, OnHit.Duration);
		}
		if (Def->MagnitudeTag.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(Def->MagnitudeTag, OnHit.Magnitude);
		}

		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
	}
}

FMFAreaHandle UMFCombatStatics::SpawnAreaEffect(AActor* Instigator, const UMFAreaEffectData* AreaData, const FVector& Location)
{
	if (!Instigator || !AreaData)
	{
		return FMFAreaHandle();
	}

	UWorld* World = Instigator->GetWorld();
	if (!World)
	{
		return FMFAreaHandle();
	}

	UMFAreaEffectSubsystem* Subsystem = World->GetSubsystem<UMFAreaEffectSubsystem>();
	if (!Subsystem)
	{
		return FMFAreaHandle();
	}

	return Subsystem->RegisterArea(Instigator, AreaData, Location);
}

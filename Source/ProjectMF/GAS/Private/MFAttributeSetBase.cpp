// Copyright ProjectMF. All Rights Reserved.

#include "MFAttributeSetBase.h"
#include "MFCombatAttributeSet.h"
#include "AbilitySystemComponent.h"

UMFAttributeSetBase::UMFAttributeSetBase()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMoveSpeed(600.f);
	InitDamage(0.f);
}

void UMFAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UMFAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute != GetDamageAttribute())
	{
		return;
	}

	// 消费元属性：读取本次伤害值后立即清零，避免重复处理
	const float IncomingDamage = GetDamage();
	SetDamage(0.f);

	if (IncomingDamage <= 0.f)
	{
		return;
	}

	// 读取防御力（CombatAttributeSet 可选，未挂载时 Defense = 0）
	float Defense = 0.f;
	float FleeThreshold = 0.3f;
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		if (const UMFCombatAttributeSet* CombatSet = ASC->GetSet<UMFCombatAttributeSet>())
		{
			Defense       = CombatSet->GetDefense();
			FleeThreshold = CombatSet->GetFleeThreshold();
		}
	}

	// 最终伤害至少为 1，防止防御力过高导致免疫
	const float FinalDamage = FMath::Max(IncomingDamage - Defense, 1.f);
	const float NewHealth   = FMath::Clamp(GetHealth() - FinalDamage, 0.f, GetMaxHealth());

	SetHealth(NewHealth);
	OnHealthChanged.Broadcast(NewHealth);

	if (NewHealth <= 0.f)
	{
		OnDeath.Broadcast();
	}
	else if (GetMaxHealth() > 0.f && NewHealth < GetMaxHealth() * FleeThreshold)
	{
		OnLowHealth.Broadcast(NewHealth);
	}
}

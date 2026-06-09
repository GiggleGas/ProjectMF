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
	InitHealing(0.f);
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

	// 回复：与伤害对称的 meta 路径
	if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		const float IncomingHeal = GetHealing();
		SetHealing(0.f);                       // 消费元属性，避免重复处理
		if (IncomingHeal <= 0.f)
		{
			return;
		}

		const float OldHealth = GetHealth();
		const float NewHealth = FMath::Clamp(OldHealth + IncomingHeal, 0.f, GetMaxHealth());
		SetHealth(NewHealth);
		OnHealthChanged.Broadcast(OldHealth, NewHealth);   // 受击闪红 guard 在掉血，回血改为闪绿
		return;
	}

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

	// 读取防御力 / 易伤系数（CombatAttributeSet 可选，未挂载时取缺省）
	float Defense = 0.f;
	float FleeThreshold = 0.3f;
	float IncomingMult = 1.f;
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		if (const UMFCombatAttributeSet* CombatSet = ASC->GetSet<UMFCombatAttributeSet>())
		{
			Defense       = CombatSet->GetDefense();
			FleeThreshold = CombatSet->GetFleeThreshold();
			IncomingMult  = CombatSet->GetIncomingDamageMultiplier();
		}
	}

	// 先减防御并保底 1（防御过高不至于免疫），再乘易伤系数（系数 0 = 故意免疫）
	const float FinalDamage = FMath::Max(IncomingDamage - Defense, 1.f) * IncomingMult;
	const float OldHealth   = GetHealth();
	const float NewHealth   = FMath::Clamp(OldHealth - FinalDamage, 0.f, GetMaxHealth());

	SetHealth(NewHealth);
	OnHealthChanged.Broadcast(OldHealth, NewHealth);

	if (NewHealth <= 0.f)
	{
		OnDeath.Broadcast();
	}
	else if (GetMaxHealth() > 0.f && NewHealth < GetMaxHealth() * FleeThreshold)
	{
		OnLowHealth.Broadcast(NewHealth);
	}
}

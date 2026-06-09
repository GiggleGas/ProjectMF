// Copyright ProjectMF. All Rights Reserved.

#include "MFCombatAttributeSet.h"

UMFCombatAttributeSet::UMFCombatAttributeSet()
{
	// 玩家默认无攻击/防御，宠物和Boss通过 GE_CharacterInit 覆盖这些值
	InitAttack(0.f);
	InitDefense(0.f);
	InitFleeThreshold(0.3f);

	// 伤害修正系数默认 1（无修正），由 buff/debuff GE 临时改写
	InitIncomingDamageMultiplier(1.f);
	InitOutgoingDamageMultiplier(1.f);
}

void UMFCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetAttackAttribute() || Attribute == GetDefenseAttribute()
		|| Attribute == GetIncomingDamageMultiplierAttribute()
		|| Attribute == GetOutgoingDamageMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetFleeThresholdAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
}

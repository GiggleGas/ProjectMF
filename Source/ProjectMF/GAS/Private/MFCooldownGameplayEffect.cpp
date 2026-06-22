// Copyright ProjectMF. All Rights Reserved.

#include "MFCooldownGameplayEffect.h"
#include "MFGameplayTags.h"

UMFCooldownGameplayEffect::UMFCooldownGameplayEffect()
{
	// 有时长，时长由调用方通过 SetByCaller(MF.Data.Cooldown) 提供。
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSBC;
	DurationSBC.DataTag = MFGameplayTags::Data_Cooldown;
	DurationMagnitude   = FGameplayEffectModifierMagnitude(DurationSBC);
}

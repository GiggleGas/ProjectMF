// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPetGameplayAbility.h"
#include "GA_Taunt.generated.h"

/**
 * UGA_Taunt — 嘲讽（宠物技能，瞬发）。
 *
 * 激活瞬间：找 TauntRadius 内所有**敌对** AI，逐个调用其威胁组件 ApplyTaunt(施法宠, TauntDuration)，
 * 强制它们锁定施法宠一段时间；随即 EndAbility（纯瞬发，不挂持续）。持续计时在每个被嘲讽敌人身上，各自到期恢复。
 *
 * 逐敌定向（无全局 tag）：只影响施法瞬间半径内的敌人，避免"路过被误锁"。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_Taunt : public UMFPetGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Taunt();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*            TriggerEventData) override;

protected:
	/** 嘲讽半径（cm）：此范围内的敌对 AI 被强制锁定施法宠。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Taunt", meta = (ClampMin = "0.0"))
	float TauntRadius = 600.f;

	/** 嘲讽持续（秒）：被嘲讽敌人保持锁定施法宠的时长。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Taunt", meta = (ClampMin = "0.1"))
	float TauntDuration = 4.f;
};

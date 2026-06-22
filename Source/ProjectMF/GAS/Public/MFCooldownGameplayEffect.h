// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MFCooldownGameplayEffect.generated.h"

/**
 * UMFCooldownGameplayEffect — 所有 AI 技能共享的冷却 GE（纯 C++，无需蓝图）。
 *
 * 构造里配成「有时长 + 时长由 SetByCaller(MF.Data.Cooldown) 提供」，不配静态 GrantedTags。
 * 由 UMFPetGameplayAbility::ApplyCooldown 写入时长并动态授予该技能自身的 AbilityTag 作为
 * 冷却身份标签；CanActivateAbility 据此拦截冷却中的激活。
 *
 * 设计：一个 GE 通吃所有技能，技能侧只需在数据资产里填 CooldownSeconds，零额外资产。
 */
UCLASS()
class PROJECTMF_API UMFCooldownGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMFCooldownGameplayEffect();
};

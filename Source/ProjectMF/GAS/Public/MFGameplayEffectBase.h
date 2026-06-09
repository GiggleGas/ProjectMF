// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "MFGameplayEffectBase.generated.h"

/**
 * 所有 ProjectMF GameplayEffect 的基类。
 *
 * 作用：给每个效果一个统一的「身份标签」EffectTag（如 Effect.Burn / Effect.Root），
 * 供区域子系统（UMFAreaEffectSubsystem）与连携子系统（UMFComboSubsystem）识别
 * 「目标当前身上有哪些效果」。
 *
 * 用法：
 *   - 具体效果做成本类的蓝图子类（GE_Burn / GE_Freeze / GE_Slow ...），
 *     在 Class Defaults 里设置 EffectTag。
 *   - 区域子系统施加某个 GE 时，从其 CDO 读取 EffectTag 来跟踪目标身上的活动效果；
 *     结算完后交给 Combo 子系统按 (TagA, TagB) 查表取新 GE。
 *
 * 说明：本类只负责「身份标识」。具体的属性修改 / 持续时间 / 周期 / 授予标签等，
 * 仍在各 GE 蓝图资产上配置——其中 DurationPolicy 决定区域的施加模式
 * （Instant = 每 tick 施加；Duration = 进入施加、离开移除）。
 */
UCLASS()
class PROJECTMF_API UMFGameplayEffectBase : public UGameplayEffect
{
	GENERATED_BODY()

public:
	/** 效果身份标签（如 Effect.Burn）。区域 / Combo 子系统据此识别效果种类。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MF|Effect")
	FGameplayTag EffectTag;

	/** 读取效果身份标签。 */
	UFUNCTION(BlueprintPure, Category = "MF|Effect")
	FGameplayTag GetEffectTag() const { return EffectTag; }
};

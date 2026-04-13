// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "GA_SummonPet.generated.h"

/**
 * GA_SummonPet — 宠物召唤 / 召回技能
 *
 * 激活方式：
 *   通过 GameplayEvent (MF.Ability.SummonPet) 触发。
 *   EventMagnitude 携带 slot 序号（1-5）。
 *
 *   当前触发源：MFCharacter 的 Demo 按键绑定（1-5 键）。
 *   未来触发源：GA_PetWheel（轮盘确认后发出同一事件）。
 *
 * 逻辑：
 *   slot 有宠物 && bIsActive == false → 查 NavMesh 随机点 → SummonPet
 *   slot 有宠物 && bIsActive == true  → RecallPet
 *   slot 为空                         → 取消
 *
 * 配置：
 *   在 BP_GA_SummonPet CDO 中设置 MinSummonRadius / MaxSummonRadius / NavQueryRetries。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_SummonPet : public UMFGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_SummonPet();

	// -----------------------------------------------------------------------
	// 配置
	// -----------------------------------------------------------------------

	/** 召唤点离玩家的最小距离（单位：cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SummonPet|Config", meta = (ClampMin = 0.f))
	float MinSummonRadius = 200.f;

	/** 召唤点离玩家的最大距离（单位：cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SummonPet|Config", meta = (ClampMin = 0.f))
	float MaxSummonRadius = 500.f;

	/** NavMesh 投影失败时的最大重试次数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SummonPet|Config", meta = (ClampMin = 1))
	int32 NavQueryRetries = 5;

	// -----------------------------------------------------------------------
	// UGameplayAbility interface
	// -----------------------------------------------------------------------

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*            TriggerEventData) override;

private:
	/**
	 * 在以 PlayerLocation 为圆心、[MinSummonRadius, MaxSummonRadius] 为环形区域内，
	 * 随机选取一个 NavMesh 上的可达点。
	 * @return true = 成功找到点并写入 OutLocation；false = 重试耗尽。
	 */
	bool FindSummonLocation(const FVector& PlayerLocation, FVector& OutLocation) const;
};

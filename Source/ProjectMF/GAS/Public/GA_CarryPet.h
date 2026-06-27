// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPlayerGameplayAbility.h"
#include "GA_CarryPet.generated.h"

class AMFPetBase;
class UMFPlayerConfig;

/**
 * GA_CarryPet — 抱起 / 移动宠物（玩家技能）。
 *
 * 切换式：按一次激活 → 就近抱起一只友方存活宠物（attach 到玩家 + 宠物进"被抱"态 + 玩家负重减速）；
 *         再按一次 / 取消 / 死亡 → 放下（detach + 宠物退"被抱"态 + 恢复玩家移速）。
 *
 * 输入侧：AMFCharacter::HandleCarryPet 按 State.CarryingPet 切换 激活 / 取消。
 * 被抱宠的免伤/脱战/打断 StateTree 等逻辑在 AMFPetBase::BeginCarried/EndCarried。
 *
 * 复活技能（后续）以本类为前身：目标改"死亡宠物" + 读条 + 复活。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_CarryPet : public UMFPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_CarryPet();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*            TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool                                 bReplicateEndAbility,
		bool                                 bWasCancelled) override;

private:
	/** 就近找一只可抱的宠物：友方(Team.Player) + 存活 + 未被抱 + 在 CarryReach 内。无则 nullptr。 */
	AMFPetBase* FindCarriablePet() const;

	const UMFPlayerConfig* GetPlayerConfig() const;

	/** 当前抱着的宠物（弱引用，宠物意外销毁自动失效）。 */
	TWeakObjectPtr<AMFPetBase> CarriedPet;
};

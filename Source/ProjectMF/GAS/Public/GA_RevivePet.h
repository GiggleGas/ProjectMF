// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPlayerGameplayAbility.h"
#include "GA_RevivePet.generated.h"

class AMFPetBase;
class UMFPlayerConfig;

/**
 * GA_RevivePet — 复活濒死宠物（玩家技能，建于抱宠之上）。
 *
 * 切换式：按一次 → 就近抱起一只**濒死**友方宠（attach 到玩家 + 暂停其 bleed-out + 起复活读条 + 玩家减速）；
 *         读条满 → 宠物回血回场（pet 自身 ReviveFromDowned，广播 OnRevived）；
 *         再按一次 / 死亡 → 取消（放下，bleed-out 恢复倒数）。
 *
 * 濒死宠在 EnterDowned 时已关碰撞=免伤，故本技能不走 BeginCarried 的那套（自己 attach 即可）。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_RevivePet : public UMFPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_RevivePet();

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
	/** 就近找一只濒死友方宠（Team.Player + IsDowned + 在 CarryReach 内）。无则 nullptr。 */
	AMFPetBase* FindDownedPet() const;

	const UMFPlayerConfig* GetPlayerConfig() const;

	/** 绑定到 pet.OnRevived：读条完成 → 结束本技能（pet 已自行回血）。 */
	void OnPetRevived();

	TWeakObjectPtr<AMFPetBase> RevivingPet;
	FDelegateHandle            RevivedHandle;
};

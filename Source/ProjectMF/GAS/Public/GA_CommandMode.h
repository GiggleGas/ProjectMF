// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFPlayerGameplayAbility.h"
#include "GA_CommandMode.generated.h"

class UMFCommandComponent;

/**
 * GA_CommandMode — 命令模式 / 林克时间（玩家技能）。
 *
 * 激活：进入命令模式（经 PlayerController 的 UMFCommandComponent → 时间膨胀 + 光标 + 去色暗角 + 交互），
 *      起一个倒计时（CommandModeDuration，游戏时间——慢动作下被拉长，正是"更多思考时间"的本意）。
 * 结束（倒计时到 / 再次按键取消 / 死亡打断 共用）：退出命令模式 + 施加冷却（CommandModeCooldown），
 *      冷却期间 ActivationBlockedTags(Cooldown.Player.CommandMode) 拦截再次进入。
 *
 * 时长/冷却配置读自 UMFPlayerConfig（配置进 Config，蓝图不写）。
 * 输入侧：AMFCharacter::HandleCommandMode 按 State.CommandMode 切换激活/取消。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_CommandMode : public UMFPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_CommandMode();

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

	/** 命令模式倒计时剩余（秒，游戏时间）；未激活返回 0。供 HUD 显示。 */
	UFUNCTION(BlueprintPure, Category = "Command")
	float GetTimeRemaining() const;

private:
	void OnDurationExpired();

	UMFCommandComponent* GetCommandComponent() const;
	float GetConfiguredDuration() const;
	float GetConfiguredCooldown() const;

	FTimerHandle DurationTimer;
};

// Copyright ProjectMF. All Rights Reserved.

#include "GA_CommandMode.h"

#include "MFCommandComponent.h"
#include "MFCooldownGameplayEffect.h"
#include "MFTimeControlSubsystem.h"
#include "MFGameplayTags.h"
#include "MFPlayerController.h"
#include "MFPlayerConfig.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"

UGA_CommandMode::UGA_CommandMode()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Player_CommandMode));

	// 激活期间持有命令模式态（供输入 toggle 判断）。
	ActivationOwnedTags.AddTag(MFGameplayTags::State_CommandMode);

	// 冷却中 / 死亡时不能进入命令模式。
	ActivationBlockedTags.AddTag(MFGameplayTags::Cooldown_Player_CommandMode);
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Dead);
}

void UGA_CommandMode::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UMFCommandComponent* Cmd = GetCommandComponent();
	if (!Cmd)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[GA_CommandMode] 无 UMFCommandComponent（PlayerController 缺组件）— 取消。"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Cmd->EnterCommandMode();

	// 倒计时按**真实时间**：世界计时器走游戏时间，会被时间膨胀拉长，
	// 所以把游戏时长 = 真实时长 × 当前时间倍率（膨胀下游戏时间走得慢，正好抵消 → 真实约 Duration 秒触发）。
	if (UWorld* World = GetWorld())
	{
		float Dilation = 1.f;
		if (const UMFTimeControlSubsystem* TC = World->GetSubsystem<UMFTimeControlSubsystem>())
		{
			Dilation = TC->GetEffectiveDilation(); // EnterCommandMode 已请求减速，此处反映生效倍率
		}
		const float GameDuration = FMath::Max(GetConfiguredDuration() * Dilation, 0.05f);

		World->GetTimerManager().SetTimer(
			DurationTimer, this, &UGA_CommandMode::OnDurationExpired, GameDuration, false);
	}
	// 保持 Running 直到倒计时/取消/打断。
}

void UGA_CommandMode::OnDurationExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_CommandMode::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimer);
	}

	if (UMFCommandComponent* Cmd = GetCommandComponent())
	{
		Cmd->ExitCommandMode();
	}

	// 退出即进冷却：施加一段时长的冷却身份标签（ActivationBlockedTags 据此拦截再进）。
	const float Cooldown = GetConfiguredCooldown();
	if (Cooldown > 0.f)
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UMFCooldownGameplayEffect::StaticClass(), 1.f);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(MFGameplayTags::Data_Cooldown, Cooldown);
			Spec.Data->DynamicGrantedTags.AddTag(MFGameplayTags::Cooldown_Player_CommandMode);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float UGA_CommandMode::GetTimeRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	// 计时器剩余是游戏时间；换算回真实秒（÷ 当前倍率）供 HUD 显示真实倒计时。
	const float GameRemaining = World->GetTimerManager().GetTimerRemaining(DurationTimer);
	if (GameRemaining <= 0.f)
	{
		return 0.f;
	}

	float Dilation = 1.f;
	if (const UMFTimeControlSubsystem* TC = World->GetSubsystem<UMFTimeControlSubsystem>())
	{
		Dilation = TC->GetEffectiveDilation();
	}
	return (Dilation > 0.f) ? GameRemaining / Dilation : GameRemaining;
}

// ============================================================================
// Helpers — 从 PlayerController / PlayerConfig 取
// ============================================================================

UMFCommandComponent* UGA_CommandMode::GetCommandComponent() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (AMFPlayerController* PC = Cast<AMFPlayerController>(Info->PlayerController.Get()))
		{
			return PC->GetCommandComponent();
		}
	}
	return nullptr;
}

float UGA_CommandMode::GetConfiguredDuration() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (const AMFPlayerController* PC = Cast<AMFPlayerController>(Info->PlayerController.Get()))
		{
			if (const UMFPlayerConfig* Cfg = PC->GetPlayerConfig())
			{
				return Cfg->CommandModeDuration;
			}
		}
	}
	return 5.f;
}

float UGA_CommandMode::GetConfiguredCooldown() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (const AMFPlayerController* PC = Cast<AMFPlayerController>(Info->PlayerController.Get()))
		{
			if (const UMFPlayerConfig* Cfg = PC->GetPlayerConfig())
			{
				return Cfg->CommandModeCooldown;
			}
		}
	}
	return 8.f;
}

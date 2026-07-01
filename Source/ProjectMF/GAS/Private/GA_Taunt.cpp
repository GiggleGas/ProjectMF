// Copyright ProjectMF. All Rights Reserved.

#include "GA_Taunt.h"

#include "MFAICharacter.h"
#include "MFThreatComponent.h"
#include "MFFactionStatics.h"
#include "MFGameplayTags.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"

UGA_Taunt::UGA_Taunt()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pet_Taunt));
}

void UGA_Taunt::ActivateAbility(
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

	AActor*                  Avatar  = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SelfASC = GetAbilitySystemComponentFromActorInfo();
	UWorld*                  World   = Avatar ? Avatar->GetWorld() : nullptr;

	if (Avatar && SelfASC && World)
	{
		const FVector Origin   = Avatar->GetActorLocation();
		const float   RadiusSq = TauntRadius * TauntRadius;
		int32         Count    = 0;

		for (TActorIterator<AMFAICharacter> It(World); It; ++It)
		{
			AMFAICharacter* AI = *It;
			if (!AI || AI == Avatar) { continue; }
			if (FVector::DistSquared(AI->GetActorLocation(), Origin) > RadiusSq) { continue; }

			// 只嘲讽敌对 AI（faction-auto）。
			if (!UMFFactionStatics::AreHostile(SelfASC, AI->GetAbilitySystemComponent())) { continue; }

			if (UMFThreatComponent* Threat = AI->FindComponentByClass<UMFThreatComponent>())
			{
				Threat->ApplyTaunt(Avatar, TauntDuration);
				++Count;
			}
		}

		MF_LOG(LogMFAI, TEXT("[Taunt] %s 嘲讽半径 %.0f 内 %d 个敌人，持续 %.1fs。"),
			*Avatar->GetName(), TauntRadius, Count, TauntDuration);
	}

	// 瞬发：立即结束（持续计时在各被嘲讽敌人身上）。
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

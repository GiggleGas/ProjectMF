// Copyright ProjectMF. All Rights Reserved.

#include "GA_RevivePet.h"

#include "MFPetBase.h"
#include "MFGameplayTags.h"
#include "MFPlayerController.h"
#include "MFPlayerConfig.h"
#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UGA_RevivePet::UGA_RevivePet()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Player_RevivePet));

	// 复活读条期间持有（供输入切换判断）。
	ActivationOwnedTags.AddTag(MFGameplayTags::State_RevivingPet);

	// 死亡 / 已在抱宠时不能复活。
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(MFGameplayTags::State_CarryingPet);
}

void UGA_RevivePet::ActivateAbility(
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

	AActor*     Avatar = GetAvatarActorFromActorInfo();
	AMFPetBase* Pet    = FindDownedPet();
	if (!Avatar || !Pet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RevivingPet = Pet;

	const UMFPlayerConfig* Cfg = GetPlayerConfig();

	// 抱起濒死宠（已关碰撞免伤）：挂到玩家头顶。
	const FVector Offset = Cfg ? Cfg->CarryHoldOffset : FVector(0.f, 0.f, 80.f);
	Pet->AttachToActor(Avatar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Pet->SetActorRelativeLocation(Offset);

	// 暂停 bleed-out + 起复活读条；读条完成由 pet 自身广播 OnRevived。
	Pet->BeginRevive(Cfg ? Cfg->ReviveChannelDuration : 3.f);
	RevivedHandle = Pet->OnRevived.AddUObject(this, &UGA_RevivePet::OnPetRevived);

	// 玩家负重减速（复用抱宠系数）。
	if (AMFCharacterBase* PlayerChar = Cast<AMFCharacterBase>(Avatar))
	{
		PlayerChar->SetMoveSpeedMultiplier(Cfg ? Cfg->CarrySpeedMultiplier : 0.5f);
	}

	MF_LOG(LogMFAI, TEXT("[RevivePet] 抱起濒死宠 %s 开始复活读条"), *Pet->GetName());
	// 保持 Running：读条完成（OnPetRevived）或取消（EndAbility）结束。
}

void UGA_RevivePet::OnPetRevived()
{
	// 读条完成（pet 已回血回场）→ 正常结束本技能。
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_RevivePet::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	if (AMFPetBase* Pet = RevivingPet.Get())
	{
		Pet->OnRevived.Remove(RevivedHandle);
		Pet->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// 仍濒死 = 复活未完成被取消 → 恢复 bleed-out 倒数（继续濒死）。
		if (Pet->IsDowned())
		{
			Pet->CancelRevive();
		}
	}
	RevivedHandle.Reset();
	RevivingPet.Reset();

	// 恢复玩家原速。
	if (AMFCharacterBase* PlayerChar = Cast<AMFCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		PlayerChar->SetMoveSpeedMultiplier(1.f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AMFPetBase* UGA_RevivePet::FindDownedPet() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World  = Avatar ? Avatar->GetWorld() : nullptr;
	if (!World || !Avatar)
	{
		return nullptr;
	}

	const UMFPlayerConfig* Cfg = GetPlayerConfig();
	const float Reach = Cfg ? Cfg->CarryReach : 200.f;
	const FVector Origin = Avatar->GetActorLocation();

	AMFPetBase* Best   = nullptr;
	float       BestSq = Reach * Reach;
	for (TActorIterator<AMFPetBase> It(World); It; ++It)
	{
		AMFPetBase* Pet = *It;
		if (!Pet || !Pet->IsDowned()) { continue; }

		const UAbilitySystemComponent* PetASC = Pet->GetAbilitySystemComponent();
		if (!PetASC || !PetASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player)) { continue; }

		const float DistSq = FVector::DistSquared(Pet->GetActorLocation(), Origin);
		if (DistSq <= BestSq)
		{
			BestSq = DistSq;
			Best   = Pet;
		}
	}
	return Best;
}

const UMFPlayerConfig* UGA_RevivePet::GetPlayerConfig() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (const AMFPlayerController* PC = Cast<AMFPlayerController>(Info->PlayerController.Get()))
		{
			return PC->GetPlayerConfig();
		}
	}
	return nullptr;
}

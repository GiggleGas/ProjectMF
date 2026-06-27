// Copyright ProjectMF. All Rights Reserved.

#include "GA_CarryPet.h"

#include "MFPetBase.h"
#include "MFGameplayTags.h"
#include "MFPlayerController.h"
#include "MFPlayerConfig.h"
#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UGA_CarryPet::UGA_CarryPet()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Player_CarryPet));

	// 抱着期间持有（供输入判断 / 被抱宠免伤等联动）。
	ActivationOwnedTags.AddTag(MFGameplayTags::State_CarryingPet);

	// 死亡时不能抱起。
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Dead);
}

void UGA_CarryPet::ActivateAbility(
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
	AMFPetBase* Pet    = FindCarriablePet();
	if (!Avatar || !Pet)
	{
		// 附近无可抱宠物：直接结束（不进入抱持态）。
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CarriedPet = Pet;

	const UMFPlayerConfig* Cfg = GetPlayerConfig();

	// 挂到玩家身上（头顶偏移）。
	const FVector Offset = Cfg ? Cfg->CarryHoldOffset : FVector(0.f, 0.f, 80.f);
	Pet->AttachToActor(Avatar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Pet->SetActorRelativeLocation(Offset);

	// 宠物进入"被抱"态（关碰撞免伤 / 停移动 / 加 State.Carried / 移动才打断 StateTree）。
	Pet->BeginCarried();

	// 玩家负重减速。
	if (AMFCharacterBase* PlayerChar = Cast<AMFCharacterBase>(Avatar))
	{
		PlayerChar->SetMoveSpeedMultiplier(Cfg ? Cfg->CarrySpeedMultiplier : 0.5f);
	}

	MF_LOG(LogMFAI, TEXT("[CarryPet] 抱起 %s"), *Pet->GetName());
	// 保持 Running 直到松开 / 取消 / 死亡。
}

void UGA_CarryPet::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();

	if (AMFPetBase* Pet = CarriedPet.Get())
	{
		Pet->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		const FVector Drop = Avatar ? Avatar->GetActorLocation() : Pet->GetActorLocation();
		Pet->EndCarried(Drop);
	}
	CarriedPet.Reset();

	// 恢复玩家原速。
	if (AMFCharacterBase* PlayerChar = Cast<AMFCharacterBase>(Avatar))
	{
		PlayerChar->SetMoveSpeedMultiplier(1.f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AMFPetBase* UGA_CarryPet::FindCarriablePet() const
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
		if (!Pet) { continue; }

		const UAbilitySystemComponent* PetASC = Pet->GetAbilitySystemComponent();
		if (!PetASC) { continue; }

		// 友方 + 存活 + 未被抱。
		if (!PetASC->HasMatchingGameplayTag(MFGameplayTags::Team_Player)) { continue; }
		if (PetASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead))   { continue; }
		if (PetASC->HasMatchingGameplayTag(MFGameplayTags::State_Carried)){ continue; }

		const float DistSq = FVector::DistSquared(Pet->GetActorLocation(), Origin);
		if (DistSq <= BestSq)
		{
			BestSq = DistSq;
			Best   = Pet;
		}
	}
	return Best;
}

const UMFPlayerConfig* UGA_CarryPet::GetPlayerConfig() const
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

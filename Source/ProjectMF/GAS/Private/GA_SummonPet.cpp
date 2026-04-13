// Copyright ProjectMF. All Rights Reserved.

#include "GA_SummonPet.h"
#include "MFGameplayTags.h"
#include "MFCharacter.h"
#include "MFInventoryComponent.h"
#include "MFPetBase.h"
#include "MFLog.h"
#include "NavigationSystem.h"
#include "Abilities/GameplayAbilityTypes.h"

// ============================================================
// 构造
// ============================================================

UGA_SummonPet::UGA_SummonPet()
{
	AbilityTags.AddTag(MFGameplayTags::Ability_SummonPet);

	// 通过 GameplayEvent 触发，EventMagnitude = slot 序号
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = MFGameplayTags::Ability_SummonPet;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 同一时间只允许一次召唤操作
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ============================================================
// ActivateAbility
// ============================================================

void UGA_SummonPet::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// --- 读取 slot 序号 ---
	if (!TriggerEventData)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_SummonPet: No TriggerEventData — must be activated via GameplayEvent."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const int32 SlotIndex = FMath::RoundToInt(TriggerEventData->EventMagnitude);
	MF_LOG(LogMFAbility, TEXT("GA_SummonPet: Triggered for slot %d."), SlotIndex);

	// --- 获取背包 ---
	AMFCharacter* Player = Cast<AMFCharacter>(GetAvatarActorFromActorInfo());
	if (!Player)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_SummonPet: Avatar is not AMFCharacter."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UMFInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (!Inventory)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_SummonPet: No InventoryComponent on player."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// --- 校验 slot ---
	const TArray<FMFPetInstance>& Pets = Inventory->GetAllPets();
	const int32 ArrayIndex = SlotIndex - 1;

	if (!Pets.IsValidIndex(ArrayIndex))
	{
		MF_LOG_WARNING(LogMFAbility, TEXT("GA_SummonPet: Slot %d is empty (total pets: %d)."),
			SlotIndex, Pets.Num());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FMFPetInstance& Pet = Pets[ArrayIndex];

	// --- 召回 or 召唤 ---
	if (Pet.bIsActive)
	{
		Inventory->RecallPet(Pet.InstanceID);
		MF_LOG(LogMFAbility, TEXT("GA_SummonPet: Recalled '%s' (slot %d)."), *Pet.PetName, SlotIndex);
	}
	else
	{
		FVector SummonLocation;
		if (!FindSummonLocation(Player->GetActorLocation(), SummonLocation))
		{
			MF_LOG_WARNING(LogMFAbility,
				TEXT("GA_SummonPet: NavMesh query failed after %d retries — summon cancelled."),
				NavQueryRetries);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		AMFPetBase* Spawned = Inventory->SummonPet(Pet.InstanceID, SummonLocation);
		if (Spawned)
		{
			MF_LOG(LogMFAbility, TEXT("GA_SummonPet: Summoned '%s' at %s."),
				*Pet.PetName, *SummonLocation.ToString());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ============================================================
// NavMesh 随机点查询
// ============================================================

bool UGA_SummonPet::FindSummonLocation(const FVector& PlayerLocation, FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		MF_LOG_WARNING(LogMFAbility, TEXT("GA_SummonPet: NavigationSystemV1 not found."));
		return false;
	}

	// 投影容差：XY 宽松，Z 较大以应对坡面
	const FVector QueryExtent(50.f, 50.f, 250.f);

	for (int32 i = 0; i < NavQueryRetries; ++i)
	{
		const float AngleRad = FMath::FRandRange(0.f, UE_TWO_PI);
		const float Distance = FMath::FRandRange(MinSummonRadius, MaxSummonRadius);

		const FVector Candidate = PlayerLocation + FVector(
			FMath::Cos(AngleRad) * Distance,
			FMath::Sin(AngleRad) * Distance,
			0.f);

		FNavLocation NavLocation;
		if (NavSys->ProjectPointToNavigation(Candidate, NavLocation, QueryExtent))
		{
			OutLocation = NavLocation.Location;
			MF_LOG(LogMFAbility, TEXT("GA_SummonPet: NavMesh point found on attempt %d: %s."),
				i + 1, *OutLocation.ToString());
			return true;
		}
	}

	return false;
}

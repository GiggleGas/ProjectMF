// Copyright ProjectMF. All Rights Reserved.

#include "MFCommandComponent.h"

#include "MFCommandTypes.h"
#include "MFPetCommandComponent.h"
#include "MFPetBase.h"
#include "MFLog.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

UMFCommandComponent::UMFCommandComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMFCommandComponent::DebugIssuePetMove(AActor* Pet, const FVector& Location)
{
	if (!Pet)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] DebugIssuePetMove: Pet is null."));
		return;
	}

	UMFPetCommandComponent* PetCmd = Pet->FindComponentByClass<UMFPetCommandComponent>();
	if (!PetCmd)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] DebugIssuePetMove: %s has no UMFPetCommandComponent."),
			*GetNameSafe(Pet));
		return;
	}

	FMFPetCommand Command;
	Command.Type           = EMFCommandType::MoveTo;
	Command.TargetLocation = Location;
	PetCmd->IssueCommand(Command);
}

void UMFCommandComponent::DebugIssuePetSkill(AActor* Pet, FGameplayTag SkillTag)
{
	if (!Pet)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] DebugIssuePetSkill: Pet is null."));
		return;
	}

	UMFPetCommandComponent* PetCmd = Pet->FindComponentByClass<UMFPetCommandComponent>();
	if (!PetCmd)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] DebugIssuePetSkill: %s has no UMFPetCommandComponent."),
			*GetNameSafe(Pet));
		return;
	}

	FMFPetCommand Command;
	Command.Type     = EMFCommandType::CastSkill;
	Command.SkillTag = SkillTag;   // 目标由技能自取（当前威胁目标）；显式目标留待 M3
	PetCmd->IssueCommand(Command);
}

void UMFCommandComponent::DebugMoveNearestPet(float X, float Y, float Z)
{
	if (AMFPetBase* Pet = FindNearestPet())
	{
		DebugIssuePetMove(Pet, FVector(X, Y, Z));
	}
}

void UMFCommandComponent::DebugSkillNearestPet(const FString& SkillTag)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*SkillTag), /*ErrorIfNotFound=*/false);
	if (!Tag.IsValid())
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] MFDebugPetSkill: invalid gameplay tag '%s'."), *SkillTag);
		return;
	}

	if (AMFPetBase* Pet = FindNearestPet())
	{
		DebugIssuePetSkill(Pet, Tag);
	}
}

AMFPetBase* UMFCommandComponent::FindNearestPet() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	const FVector Origin = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	AMFPetBase* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AMFPetBase> It(World); It; ++It)
	{
		AMFPetBase* Pet = *It;
		const float DistSq = FVector::DistSquared(Pet->GetActorLocation(), Origin);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Pet;
		}
	}

	if (!Nearest)
	{
		MF_LOG_WARNING(LogMFAI, TEXT("[Command] FindNearestPet: no AMFPetBase found in world."));
	}
	return Nearest;
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFGatherNode.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "MFCharacter.h"
#include "MFGameplayTags.h"
#include "MFLootSubsystem.h"
#include "MFLootTable.h"
#include "MFLog.h"

AMFGatherNode::AMFGatherNode()
{
	// 基类关掉了 tick；采集点需要 tick 驱动读条检测。
	PrimaryActorTick.bCanEverTick = true;

	BlockingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingBox"));
	BlockingBox->SetupAttachment(GetRootComponent());
	BlockingBox->SetBoxExtent(FVector(40.f, 40.f, 80.f));
	BlockingBox->SetCollisionProfileName(TEXT("BlockAll"));
}

void AMFGatherNode::BeginPlay()
{
	Super::BeginPlay();
	RemainingHarvests = MaxHarvestCount;
}

void AMFGatherNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDepleted()) return;

	if (IsPlayerGathering())
	{
		Progress += DeltaTime;
		DrawProgressBar();
		if (Progress >= HarvestDuration)
		{
			Progress = 0.f;
			Gather();
		}
	}
	else if (!bKeepProgressOnInterrupt && Progress > 0.f)
	{
		Progress = 0.f;
	}
}

// ---------------------------------------------------------------------------
// 采集
// ---------------------------------------------------------------------------

void AMFGatherNode::Gather()
{
	if (IsDepleted()) return;

	if (UMFLootSubsystem* Loot = GetWorld()->GetSubsystem<UMFLootSubsystem>())
	{
		Loot->DropFromTable(LootTable, GetActorLocation());
	}

	--RemainingHarvests;
	MF_LOG(LogMFLoot, TEXT("%s 采集产出（剩余 %d 次）。"), *GetName(), RemainingHarvests);
	OnHarvested(RemainingHarvests);

	if (IsDepleted())
	{
		OnDepleted();
		if (bDestroyOnDepleted)
		{
			Destroy();
		}
		else
		{
			// 留残桩：不再阻挡也不再读条（Tick 靠 IsDepleted 早退）。
			BlockingBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

bool AMFGatherNode::IsPlayerGathering() const
{
	const AMFCharacter* Player = Cast<AMFCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player) return false;

	if (FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) >
		FMath::Square(InteractRadius))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(MFGameplayTags::State_Picking);
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void AMFGatherNode::DrawProgressBar() const
{
#if ENABLE_DRAW_DEBUG
	const float Alpha = GetHarvestProgress();
	if (Alpha <= 0.f) return;

	const FVector Base  = GetActorLocation() + FVector(0.f, 0.f, 120.f);
	const float   Width = 60.f;
	const FVector Left  = Base - FVector(Width * 0.5f, 0.f, 0.f);

	DrawDebugLine(GetWorld(), Left, Left + FVector(Width, 0.f, 0.f), FColor::Silver, false, -1.f, 0, 2.f);
	DrawDebugLine(GetWorld(), Left, Left + FVector(Width * Alpha, 0.f, 0.f), FColor::Green, false, -1.f, 0, 6.f);
#endif
}

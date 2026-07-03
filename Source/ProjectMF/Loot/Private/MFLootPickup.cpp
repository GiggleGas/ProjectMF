// Copyright ProjectMF. All Rights Reserved.

#include "MFLootPickup.h"
#include "Components/SphereComponent.h"
#include "MFCharacter.h"
#include "MFInventoryComponent.h"
#include "MFLog.h"

AMFLootPickup::AMFLootPickup()
{
	// 基类关掉了 tick；掉落物需要 tick 驱动散开/吸附插值。
	PrimaryActorTick.bCanEverTick = true;

	MagnetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetSphere"));
	MagnetSphere->SetupAttachment(GetRootComponent());
	MagnetSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MagnetSphere->SetCollisionObjectType(ECC_WorldDynamic);
	MagnetSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	MagnetSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MagnetSphere->SetGenerateOverlapEvents(true);
}

void AMFLootPickup::BeginPlay()
{
	Super::BeginPlay();

	MagnetSphere->SetSphereRadius(MagnetRadius);
	MagnetSphere->OnComponentBeginOverlap.AddDynamic(this, &AMFLootPickup::OnMagnetBeginOverlap);

	if (Lifetime > 0.f)
	{
		SetLifeSpan(Lifetime);
	}
}

void AMFLootPickup::InitLoot(FName InItemID, int32 InCount, const FVector& LandLocation)
{
	ItemID = InItemID;
	Count  = FMath::Max(InCount, 1);

	ScatterStart = GetActorLocation();
	ScatterEnd   = LandLocation;
	ScatterTime  = 0.f;
	State = (ScatterDuration > 0.f && !ScatterStart.Equals(ScatterEnd, 1.f))
		? EPickupState::Scatter
		: EPickupState::Idle;

	OnLootInitialized(ItemID, Count);
}

// ---------------------------------------------------------------------------
// Tick 状态机
// ---------------------------------------------------------------------------

void AMFLootPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (State)
	{
	case EPickupState::Scatter: TickScatter(DeltaTime); break;
	case EPickupState::Idle:    TickIdle();             break;
	case EPickupState::Magnet:  TickMagnet(DeltaTime);  break;
	}
}

void AMFLootPickup::TickScatter(float DeltaTime)
{
	ScatterTime += DeltaTime;
	const float Alpha = FMath::Clamp(ScatterTime / ScatterDuration, 0.f, 1.f);

	// 线性插值 + 正弦拱起 = 小抛物线。
	FVector Pos = FMath::Lerp(ScatterStart, ScatterEnd, Alpha);
	Pos.Z += FMath::Sin(Alpha * PI) * ScatterArcHeight;
	SetActorLocation(Pos);

	if (Alpha >= 1.f)
	{
		State = EPickupState::Idle;
	}
}

void AMFLootPickup::TickIdle()
{
	// 背包满回落后：冷却结束且玩家仍在感应半径内 → 重新吸附。
	// （新玩家进入走 OnMagnetBeginOverlap，这里只处理"留在范围内"的重试。）
	if (!MagnetTarget.IsValid()) return;
	if (GetWorld()->GetTimeSeconds() < RetryReadyTime) return;

	if (MagnetSphere->IsOverlappingActor(MagnetTarget.Get()))
	{
		StartMagnet(MagnetTarget.Get());
	}
	else
	{
		MagnetTarget.Reset();
	}
}

void AMFLootPickup::TickMagnet(float DeltaTime)
{
	AMFCharacter* Target = MagnetTarget.Get();
	if (!Target)
	{
		State = EPickupState::Idle;
		return;
	}

	MagnetSpeed += MagnetAcceleration * DeltaTime;

	const FVector TargetPos = Target->GetActorLocation();
	const FVector Current   = GetActorLocation();
	const FVector Delta     = TargetPos - Current;
	const float   Dist      = Delta.Size();

	if (Dist <= AbsorbDistance || Dist <= MagnetSpeed * DeltaTime)
	{
		SetActorLocation(TargetPos);
		TryGiveTo(Target);
		return;
	}

	SetActorLocation(Current + Delta / Dist * MagnetSpeed * DeltaTime);
}

// ---------------------------------------------------------------------------
// 吸附与入包
// ---------------------------------------------------------------------------

void AMFLootPickup::OnMagnetBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (State != EPickupState::Idle) return;

	AMFCharacter* Player = Cast<AMFCharacter>(OtherActor);
	if (!Player) return;

	// 冷却中只记录目标，重试交给 TickIdle。
	MagnetTarget = Player;
	if (GetWorld()->GetTimeSeconds() >= RetryReadyTime)
	{
		StartMagnet(Player);
	}
}

void AMFLootPickup::StartMagnet(AMFCharacter* Target)
{
	MagnetTarget = Target;
	MagnetSpeed  = MagnetInitialSpeed;
	State        = EPickupState::Magnet;
}

void AMFLootPickup::TryGiveTo(AMFCharacter* Target)
{
	UMFInventoryComponent* Inventory = Target ? Target->GetInventoryComponent() : nullptr;
	const int32 Added = Inventory ? Inventory->AddResource(ItemID, Count) : 0;

	if (Added >= Count)
	{
		Destroy();
		return;
	}

	// 背包满（或物品未注册进 ItemDatabase）：扣除已入包部分，原地回落，冷却后重试。
	Count -= Added;
	if (Added == 0)
	{
		MF_LOG_WARNING(LogMFLoot, TEXT("拾取 %s x%d 失败（背包满或 ItemDatabase 未注册），回落待重试。"),
			*ItemID.ToString(), Count);
	}
	State         = EPickupState::Idle;
	RetryReadyTime = GetWorld()->GetTimeSeconds() + RetryCooldown;
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFLootPickup.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"
#include "MFInventoryComponent.h"
#include "MFLog.h"

AMFLootPickup::AMFLootPickup()
{
	// 需要 tick 驱动出生散开 + billboard。
	PrimaryActorTick.bCanEverTick = true;

	// 场景外观：单张 Sprite（掉落图标一般静态），资产在 InitLoot 时按物品填入。
	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
	SpriteComponent->SetupAttachment(GetRootComponent());
	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMFLootPickup::BeginPlay()
{
	Super::BeginPlay();

	if (Lifetime > 0.f)
	{
		SetLifeSpan(Lifetime);
	}
}

void AMFLootPickup::InitLoot(int32 InItemID, int32 InCount, const FVector& LandLocation)
{
	ItemID = InItemID;
	Count  = FMath::Max(InCount, 1);

	// 场景外观由物品总表的 WorldSprite 决定——一个 pickup 类通吃所有物品，
	// 加物品只需在总表配 WorldSprite，无需为每种物品做一个掉落物蓝图。
	if (const FMFItemDef* Def = UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID))
	{
		if (Def->WorldSprite)
		{
			SpriteComponent->SetSprite(Def->WorldSprite);
		}
		else
		{
			MF_LOG_WARNING(LogMFLoot, TEXT("物品 #%d 未配 WorldSprite，掉落物将不可见。"), ItemID);
		}
	}

	ScatterStart = GetActorLocation();
	ScatterEnd   = LandLocation;
	ScatterTime  = 0.f;
	State = (ScatterDuration > 0.f && !ScatterStart.Equals(ScatterEnd, 1.f))
		? EPickupState::Scatter
		: EPickupState::Idle;

	OnLootInitialized(ItemID, Count);
}

int32 AMFLootPickup::TryPickUpInto(UMFInventoryComponent* Inv)
{
	if (!Inv) return 0;

	const int32 Added = Inv->AddResource(ItemID, Count);
	if (Added >= Count)
	{
		MF_LOG(LogMFLoot, TEXT("拾取 #%d x%d 入包。"), ItemID, Count);
		Destroy();
		return Added;
	}

	// 背包满 / 只装下一部分：扣掉已入的，剩余重新抛一次散落（弹起重落，明确反馈）。
	Count -= Added;
	MF_LOG(LogMFLoot, TEXT("拾取 #%d 背包满，剩 x%d 弹回。"), ItemID, Count);
	BounceOut();
	return Added;
}

void AMFLootPickup::BounceOut()
{
	const FVector Cur = GetActorLocation();

	const FVector2D Off = FMath::RandPointInCircle(BounceRadius);
	FVector Land = Cur + FVector(Off.X, Off.Y, 0.f);

	// 向下 trace 对齐地面。
	FHitResult Hit;
	const FVector TraceStart = Land + FVector(0, 0, 100.f);
	const FVector TraceEnd   = Land - FVector(0, 0, 1000.f);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility))
	{
		Land.Z = Hit.ImpactPoint.Z + 5.f;
	}

	ScatterStart = Cur;
	ScatterEnd   = Land;
	ScatterTime  = 0.f;
	State        = EPickupState::Scatter;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AMFLootPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State == EPickupState::Scatter)
	{
		TickScatter(DeltaTime);
	}

	UpdateBillboard();
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

void AMFLootPickup::UpdateBillboard()
{
	const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!PCM) return;

	FVector ToCam = PCM->GetCameraLocation() - GetActorLocation();
	ToCam.Z = 0.f;
	if (ToCam.IsNearlyZero()) return;

	SetActorRotation(FRotator(0.f, ToCam.Rotation().Yaw + BillboardYawOffset, 0.f));
}

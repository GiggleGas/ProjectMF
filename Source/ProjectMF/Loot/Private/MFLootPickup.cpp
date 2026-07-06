// Copyright ProjectMF. All Rights Reserved.

#include "MFLootPickup.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"
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

void AMFLootPickup::PickUp()
{
	// 虚空拾取（当前无背包）：捡起即消失。
	// 将来接背包时改为 AddResource(ItemID, Count) 成功后再 Destroy。
	MF_LOG(LogMFLoot, TEXT("拾取 #%d x%d（虚空，无背包）。"), ItemID, Count);
	Destroy();
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

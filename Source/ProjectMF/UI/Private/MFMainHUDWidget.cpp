// Copyright ProjectMF. All Rights Reserved.

#include "MFMainHUDWidget.h"
#include "MFPetSlotWidget.h"
#include "MFOverheadWidget.h"
#include "MFCharacter.h"
#include "MFInventoryComponent.h"
#include "MFGameLoopConfig.h"
#include "MFPetBase.h"
#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Blueprint/UserWidget.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMFMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AMFGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMFGameMode>() : nullptr;
	if (!GM) return;

	BoundGameMode = GM;
	GM->OnPhaseChanged.AddDynamic(this,   &UMFMainHUDWidget::HandlePhaseChanged);
	GM->OnCountdownTick.AddDynamic(this,  &UMFMainHUDWidget::HandleCountdownTick);
	GM->OnBossPhaseReady.AddDynamic(this, &UMFMainHUDWidget::HandleBossPhaseReady);
	GM->OnGameResult.AddDynamic(this,     &UMFMainHUDWidget::HandleGameResult);

	// 初始状态：结算遮罩和 Boss 提示隐藏，其余常驻显示
	if (BossReadyBanner) BossReadyBanner->SetVisibility(ESlateVisibility::Hidden);
	if (ResultOverlay)   ResultOverlay->SetVisibility(ESlateVisibility::Hidden);
}

void UMFMainHUDWidget::NativeDestruct()
{
	if (AMFGameMode* GM = BoundGameMode.Get())
	{
		GM->OnPhaseChanged.RemoveDynamic(this,   &UMFMainHUDWidget::HandlePhaseChanged);
		GM->OnCountdownTick.RemoveDynamic(this,  &UMFMainHUDWidget::HandleCountdownTick);
		GM->OnBossPhaseReady.RemoveDynamic(this, &UMFMainHUDWidget::HandleBossPhaseReady);
		GM->OnGameResult.RemoveDynamic(this,     &UMFMainHUDWidget::HandleGameResult);
	}

	if (UMFInventoryComponent* Inv = BoundInventory.Get())
	{
		Inv->OnPetRosterChanged.RemoveDynamic(this, &UMFMainHUDWidget::OnPetRosterChanged);
		Inv->OnPetReviveTick.RemoveDynamic(this,    &UMFMainHUDWidget::OnPetReviveTick);
	}

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Player init (called by AMFPlayerController after AddToViewport)
// ---------------------------------------------------------------------------

void UMFMainHUDWidget::InitPlayerHUD(AMFCharacter* Player)
{
	if (!Player) return;

	// 玩家血条：绑定 ASC，之后受伤/回血自动更新
	if (PlayerHPBar)
	{
		PlayerHPBar->InitWithASC(Player->GetAbilitySystemComponent());
	}

	// 宠物背包：订阅花名册变更 + 缓存 DB 引用
	UMFInventoryComponent* Inv = Player->GetInventoryComponent();
	if (Inv)
	{
		BoundInventory = Inv;
		Inv->OnPetRosterChanged.AddDynamic(this, &UMFMainHUDWidget::OnPetRosterChanged);
		Inv->OnPetReviveTick.AddDynamic(this,    &UMFMainHUDWidget::OnPetReviveTick);
		RefreshPetSlots();
	}
}

// ---------------------------------------------------------------------------
// GameMode delegate handlers
// ---------------------------------------------------------------------------

void UMFMainHUDWidget::HandlePhaseChanged(EMFGamePhase NewPhase)
{
	const bool bResult = NewPhase == EMFGamePhase::Victory || NewPhase == EMFGamePhase::Defeat;
	if (ResultOverlay)   ResultOverlay->SetVisibility(bResult ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	if (BossReadyBanner) BossReadyBanner->SetVisibility(ESlateVisibility::Hidden);
}

void UMFMainHUDWidget::HandleCountdownTick(float TimeRemaining)
{
	if (!CountdownText) return;

	const int32 Total   = FMath::Max(0, FMath::FloorToInt(TimeRemaining));
	const int32 Minutes = Total / 60;
	const int32 Seconds = Total % 60;
	CountdownText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), Minutes, Seconds)));
}

void UMFMainHUDWidget::HandleBossPhaseReady()
{
	if (BossReadyBanner)
	{
		BossReadyBanner->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMFMainHUDWidget::HandleGameResult(bool bVictory)
{
	if (ResultText)
	{
		ResultText->SetText(FText::FromString(bVictory ? TEXT("胜利！") : TEXT("失败")));
	}
}

// ---------------------------------------------------------------------------
// Pet roster
// ---------------------------------------------------------------------------

void UMFMainHUDWidget::OnPetRosterChanged()
{
	RefreshPetSlots();
}

void UMFMainHUDWidget::OnPetReviveTick()
{
	// 仅刷新读秒数字，不重建卡片（避免每秒重载头像/闪烁）。
	UMFInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	for (const FMFPetInstance& Pet : Inv->GetAllPets())
	{
		if (!Pet.bIsDead) continue;

		if (TObjectPtr<UMFPetSlotWidget>* SlotPtr = SlotWidgets.Find(Pet.InstanceID))
		{
			if (*SlotPtr)
			{
				(*SlotPtr)->UpdateReviveRemaining(Pet.ReviveTimeRemaining);
			}
		}
	}
}

void UMFMainHUDWidget::RefreshPetSlots()
{
	if (!PetSlotList || !PetSlotClass) return;

	UMFInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	PetSlotList->ClearChildren();
	SlotWidgets.Reset();

	// 整张花名册：出战 / 待命 / 复活读秒 各一张卡片
	const TArray<FMFPetInstance>& Pets = Inv->GetAllPets();
	for (const FMFPetInstance& Pet : Pets)
	{
		UMFPetSlotWidget* MFPetSlot = CreateWidget<UMFPetSlotWidget>(this, PetSlotClass);
		if (!MFPetSlot) continue;

		AMFPetBase* LiveActor = Pet.bIsActive ? Inv->GetActivePetActor(Pet.InstanceID) : nullptr;
		MFPetSlot->InitWithPet(Pet, LiveActor, Inv->AIRegistry);
		PetSlotList->AddChild(MFPetSlot);
		SlotWidgets.Add(Pet.InstanceID, MFPetSlot);
	}

	// 更新宠物计数文本
	if (PetCountText)
	{
		int32 Required = 3;
		if (AMFGameMode* GM = BoundGameMode.Get())
		{
			if (GM->GameLoopConfig)
			{
				Required = GM->GameLoopConfig->PetsRequiredForEarlyTrigger;
			}
		}
		PetCountText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d 只"), Pets.Num(), Required)));
	}
}

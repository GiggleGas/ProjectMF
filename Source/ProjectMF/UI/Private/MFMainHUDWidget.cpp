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

void UMFMainHUDWidget::RefreshPetSlots()
{
	if (!PetSlotList || !PetSlotClass) return;

	UMFInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	PetSlotList->ClearChildren();

	TArray<AMFPetBase*> ActiveActors = Inv->GetActivePetActors();
	for (AMFPetBase* Pet : ActiveActors)
	{
		if (!Pet) continue;

		UMFPetSlotWidget* MFPetSlot = CreateWidget<UMFPetSlotWidget>(this, PetSlotClass);
		if (MFPetSlot)
		{
			MFPetSlot->InitWithPetActor(Pet);
			PetSlotList->AddChild(MFPetSlot);
		}
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
		const int32 Count = Inv->GetAllPets().Num();
		PetCountText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d 只"), Count, Required)));
	}
}

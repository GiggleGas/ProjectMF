// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFGameMode.h"
#include "MFMainHUDWidget.generated.h"

class UTextBlock;
class UOverlay;
class UPanelWidget;
class UMFOverheadWidget;
class UMFPetSlotWidget;
class AMFCharacter;
class UMFInventoryComponent;

/**
 * C++ base class for the player's main HUD.
 *
 * All widget slots are declared with BindWidget — Blueprint only places and
 * styles the controls; all data-driving logic lives here in C++.
 *
 * Usage:
 *   1. Create WBP_MainHUD inheriting from UMFMainHUDWidget.
 *   2. In Designer, add widgets with names that exactly match the BindWidget properties.
 *   3. Assign WBP_MainHUD to PlayerConfig->MainHUDClass on BP_PlayerController.
 *   4. Set PetSlotClass to WBP_PetSlot in the Widget Blueprint defaults.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Called by AMFPlayerController::BeginPlay after the HUD is created.
	 * Initialises the player HP bar, subscribes to inventory changes,
	 * and triggers the first pet-slot refresh.
	 */
	void InitPlayerHUD(AMFCharacter* Player);

	// -----------------------------------------------------------------------
	// Widget Bindings — name each Designer widget to match exactly
	// -----------------------------------------------------------------------

	/** 玩家血条。在 Designer 中放置 WBP_OverheadHP 并命名为 "PlayerHPBar"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMFOverheadWidget> PlayerHPBar;

	/** 倒计时文本，显示 "M:SS" 格式。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	/** 已捕获宠物数量，显示 "n / max 只"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PetCountText;

	/** 宠物卡槽容器（HorizontalBox / WrapBox 均可）。动态填入 WBP_PetSlot。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PetSlotList;

	/** "[ Tab ] 开启 Boss 战" 提示，满足条件时短暂显示。可选。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossReadyBanner;

	/** 胜负结算遮罩，Victory / Defeat 阶段显示。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> ResultOverlay;

	/** 胜负文本，显示 "胜利！" 或 "失败"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

	// -----------------------------------------------------------------------
	// Configuration
	// -----------------------------------------------------------------------

	/** 宠物卡槽 Widget 类（必须继承 UMFPetSlotWidget）。在 WBP_MainHUD Defaults 中赋值。 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Pets")
	TSubclassOf<UMFPetSlotWidget> PetSlotClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// -----------------------------------------------------------------------
	// GameMode delegate handlers
	// -----------------------------------------------------------------------

	UFUNCTION() void HandlePhaseChanged(EMFGamePhase NewPhase);
	UFUNCTION() void HandleCountdownTick(float TimeRemaining);
	UFUNCTION() void HandleBossPhaseReady();
	UFUNCTION() void HandleGameResult(bool bVictory);

	// -----------------------------------------------------------------------
	// Pet roster
	// -----------------------------------------------------------------------

	UFUNCTION() void OnPetRosterChanged();
	void RefreshPetSlots();

	// -----------------------------------------------------------------------
	// Runtime state
	// -----------------------------------------------------------------------

	UPROPERTY() TWeakObjectPtr<AMFGameMode>           BoundGameMode;
	UPROPERTY() TWeakObjectPtr<UMFInventoryComponent> BoundInventory;
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFPetSlotWidget.generated.h"

class UImage;
class UMFOverheadWidget;
class AMFPetBase;
class UMFItemDatabase;

/**
 * C++ base class for a single pet card in the CatchingPanel.
 *
 * Declares two BindWidget slots — Portrait and PetHPBar — so Blueprint only
 * handles layout and styling. InitWithPetActor sets the portrait icon and
 * wires PetHPBar to the pet's ASC for live HP tracking.
 *
 * PetHPBar reuses WBP_OverheadHP (inherits UMFOverheadWidget), the same
 * Blueprint used for AI / pet overhead bars in world space.
 *
 * Usage:
 *   1. Create WBP_PetSlot inheriting from UMFPetSlotWidget.
 *   2. Add an Image named "Portrait" and a WBP_OverheadHP named "PetHPBar".
 *   3. Assign WBP_PetSlot to WBP_MainHUD's PetSlotClass default.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFPetSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Populate portrait from the item database and bind PetHPBar to the pet's ASC.
	 * Called by UMFMainHUDWidget::RefreshPetSlots after CreateWidget.
	 */
	void InitWithPetActor(AMFPetBase* Pet, UMFItemDatabase* Database);

	// -----------------------------------------------------------------------
	// Widget Bindings — name Designer widgets to match exactly
	// -----------------------------------------------------------------------

	/** 宠物头像。在 Designer 中放置 Image 并命名为 "Portrait"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait;

	/**
	 * 宠物血条。在 Designer 中放置 WBP_OverheadHP 并命名为 "PetHPBar"。
	 * 与世界空间角色头顶的血条是同一个 Blueprint，InitWithASC 逻辑共用。
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMFOverheadWidget> PetHPBar;
};

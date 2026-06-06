// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFPetSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UDataTable;
class UMFOverheadWidget;
class AMFPetBase;
struct FMFPetInstance;

/**
 * C++ base class for a single pet card in the CatchingPanel.
 *
 * 一张卡片对应背包里的一只宠物，覆盖三种状态：
 *   出战中   — 有存活 Actor，PetHPBar 绑定其 ASC 显示实时血量。
 *   待命中   — 无 Actor，仅显示头像（HP 条隐藏）。
 *   复活读秒 — 无 Actor，头像变暗 + ReviveCountdownText 显示剩余秒数。
 *
 * 所有数据驱动逻辑都在 C++（UMFMainHUDWidget 调用 InitWithPet）。
 * Blueprint 只需在 Designer 摆好同名控件，无需连任何蓝图节点。
 *
 * Usage:
 *   1. Create WBP_PetSlot inheriting from UMFPetSlotWidget.
 *   2. Add Image "Portrait"、WBP_OverheadHP "PetHPBar"，以及可选 TextBlock "ReviveCountdownText"。
 *   3. Assign WBP_PetSlot to WBP_MainHUD's PetSlotClass default.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFPetSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 根据宠物实例数据填充整张卡片。由 UMFMainHUDWidget::RefreshPetSlots 调用。
	 * @param Inst        宠物实例（含出战/复活状态与读秒）。
	 * @param LiveActor   出战中的 Actor，待命/复活时为 nullptr。
	 * @param AIRegistry  AI 注册表，用于在无 Actor 时按 AIConfigID 取头像。
	 */
	void InitWithPet(const FMFPetInstance& Inst, AMFPetBase* LiveActor, UDataTable* AIRegistry);

	/** 仅刷新复活读秒数字（每秒一次，不重建卡片）。 */
	void UpdateReviveRemaining(float SecondsRemaining);

	// -----------------------------------------------------------------------
	// Widget Bindings — name Designer widgets to match exactly
	// -----------------------------------------------------------------------

	/** 宠物头像。在 Designer 中放置 Image 并命名为 "Portrait"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait;

	/**
	 * 宠物血条。在 Designer 中放置 WBP_OverheadHP 并命名为 "PetHPBar"。
	 * 与世界空间角色头顶的血条是同一个 Blueprint，InitWithASC 逻辑共用。
	 * 仅出战中可见，待命/复活时自动隐藏。
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMFOverheadWidget> PetHPBar;

	/** 复活读秒数字（可选）。在 Designer 中放置 TextBlock 并命名为 "ReviveCountdownText"。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReviveCountdownText;

private:
	/** 从 PetConfig 设置头像图标；Config 为空则不改动。 */
	void ApplyPortrait(const class UMFPetConfig* Config);
};

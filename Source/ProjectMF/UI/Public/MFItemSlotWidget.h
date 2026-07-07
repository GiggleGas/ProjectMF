// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UDragDropOperation;

/** 请求丢弃某格（右键 或 拖出背包松开），参数 = 格位索引。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMFItemSlotDiscard, int32, SlotIndex);

/**
 * 单个背包格子。仿 UMFPetSlotWidget：逻辑全在 C++，蓝图只摆同名 BindWidget 控件、不连节点。
 *
 * 与宠物卡槽的区别：背包**固定画满 N 格**，空格也显示（空位）。
 *
 * 丢弃：右键立即丢；或左键拖起 → 拖到背包区域外松开 = 丢（拖回背包内松开 = 放回不丢，
 * 由 UMFBackpackWidget 吸收内部 drop 实现）。
 *
 * Usage:
 *   1. 建 WBP_ItemSlot 继承本类。
 *   2. Designer 放 Image "Icon" + TextBlock "CountText"（+ 可选 Image "Background" 底框）。
 *   3. WBP_Backpack 的 ItemSlotClass 指向 WBP_ItemSlot。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 填充格子。空格（ItemID<=0 或 Count<=0）→ 隐藏 Icon/CountText，底框保留（空位）。 */
	void SetSlot(int32 InSlotIndex, int32 InItemID, int32 InCount);

	/** 请求丢弃委托（容器订阅 → DropSlot）。右键 或 拖出背包松开触发。 */
	FOnMFItemSlotDiscard OnSlotDiscardRequested;

	// -----------------------------------------------------------------------
	// Widget Bindings — name Designer widgets to match exactly
	// -----------------------------------------------------------------------

	/** 物品图标。Designer 放 Image 命名 "Icon"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Icon;

	/** 数量文本。Designer 放 TextBlock 命名 "CountText"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	/** 格子底框（空/满都常驻，作为空位的视觉载体）。可选。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Background;

protected:
	/** 空/满外观变化钩子——让蓝图定制空格淡底、满格高亮等（无需写 C++）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "ItemSlot")
	void OnSlotVisualUpdated(bool bEmpty);

	/** 拖动视觉（可选）：返回一个 widget 作拖动时跟随鼠标的图像；不实现则无视觉。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "ItemSlot")
	UWidget* OnMakeDragVisual();

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	/** 拖动在背包区域外松开（无 target 吸收）→ 丢弃。 */
	UFUNCTION() void HandleDragCancelled(UDragDropOperation* Operation);

	int32 SlotIndex = INDEX_NONE;
	int32 CurrentItemID = 0;   // 拖动时判断是否空格
};

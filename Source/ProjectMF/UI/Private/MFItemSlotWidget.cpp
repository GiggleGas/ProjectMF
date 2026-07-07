// Copyright ProjectMF. All Rights Reserved.

#include "MFItemSlotWidget.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"
#include "MFItemTypes.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/DragDropOperation.h"

void UMFItemSlotWidget::SetSlot(int32 InSlotIndex, int32 InItemID, int32 InCount)
{
	SlotIndex     = InSlotIndex;
	CurrentItemID = InItemID;
	const bool bEmpty = (InItemID <= 0 || InCount <= 0);

	if (bEmpty)
	{
		if (Icon)      Icon->SetVisibility(ESlateVisibility::Collapsed);
		if (CountText) CountText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		if (Icon)
		{
			if (const FMFItemDef* Def = UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), InItemID))
			{
				if (Def->Icon)
				{
					Icon->SetBrushFromTexture(Def->Icon);
				}
			}
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (CountText)
		{
			if (InCount > 1)
			{
				CountText->SetText(FText::AsNumber(InCount));
				CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				CountText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	OnSlotVisualUpdated(bEmpty);
}

FReply UMFItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 右键：立即丢弃。
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && SlotIndex >= 0)
	{
		OnSlotDiscardRequested.Broadcast(SlotIndex);
		return FReply::Handled();
	}

	// 左键（非空格）：启动拖动检测，松开若在背包外 = 丢弃。
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CurrentItemID > 0)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMFItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (CurrentItemID <= 0) return; // 空格不拖

	UDragDropOperation* Op = NewObject<UDragDropOperation>(this);
	Op->Payload           = this;
	Op->DefaultDragVisual = OnMakeDragVisual(); // 蓝图可选提供；返回 nullptr 则无视觉
	Op->Pivot             = EDragPivot::MouseDown;
	Op->OnDragCancelled.AddDynamic(this, &UMFItemSlotWidget::HandleDragCancelled);
	OutOperation = Op;
}

void UMFItemSlotWidget::HandleDragCancelled(UDragDropOperation* /*Operation*/)
{
	// 拖出背包区域松开（无 drop target 吸收）→ 丢弃。
	if (SlotIndex >= 0)
	{
		OnSlotDiscardRequested.Broadcast(SlotIndex);
	}
}

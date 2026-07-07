// Copyright ProjectMF. All Rights Reserved.

#include "MFBackpackWidget.h"
#include "MFItemSlotWidget.h"
#include "MFInventoryComponent.h"
#include "Components/PanelWidget.h"
#include "Blueprint/DragDropOperation.h"

void UMFBackpackWidget::InitBackpack(UMFInventoryComponent* Inv)
{
	if (!Inv) return;

	BoundInventory = Inv;
	Inv->OnInventoryChanged.AddDynamic(this, &UMFBackpackWidget::HandleInventoryChanged);

	BuildSlots(Inv->GetResourceSlots().Num());
	RefreshSlots();
}

void UMFBackpackWidget::NativeDestruct()
{
	if (UMFInventoryComponent* Inv = BoundInventory.Get())
	{
		Inv->OnInventoryChanged.RemoveDynamic(this, &UMFBackpackWidget::HandleInventoryChanged);
	}
	Super::NativeDestruct();
}

void UMFBackpackWidget::BuildSlots(int32 NumSlots)
{
	if (!SlotGrid || !ItemSlotClass) return;

	SlotGrid->ClearChildren();
	SlotWidgets.Reset();

	for (int32 i = 0; i < NumSlots; ++i)
	{
		UMFItemSlotWidget* NewSlot = CreateWidget<UMFItemSlotWidget>(this, ItemSlotClass);
		if (!NewSlot) continue;

		NewSlot->OnSlotDiscardRequested.AddDynamic(this, &UMFBackpackWidget::HandleSlotDiscard);
		SlotGrid->AddChild(NewSlot);   // 排布由容器决定（VerticalBox 单列 / WrapBox 多列…）
		SlotWidgets.Add(NewSlot);
	}
}

void UMFBackpackWidget::RefreshSlots()
{
	UMFInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	const TArray<FMFInventorySlot>& Slots = Inv->GetResourceSlots();
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (!SlotWidgets[i]) continue;

		if (Slots.IsValidIndex(i))
		{
			SlotWidgets[i]->SetSlot(i, Slots[i].ItemID, Slots[i].Count);
		}
		else
		{
			SlotWidgets[i]->SetSlot(i, 0, 0); // 空位
		}
	}
}

void UMFBackpackWidget::HandleInventoryChanged()
{
	// 容量可能在 HUD 初始化之后才就绪（Pawn BeginPlay 时序）：格子数不匹配则重建。
	if (UMFInventoryComponent* Inv = BoundInventory.Get())
	{
		if (SlotWidgets.Num() != Inv->GetResourceSlots().Num())
		{
			BuildSlots(Inv->GetResourceSlots().Num());
		}
	}
	RefreshSlots();
}

void UMFBackpackWidget::HandleSlotDiscard(int32 SlotIndex)
{
	if (UMFInventoryComponent* Inv = BoundInventory.Get())
	{
		Inv->DropSlot(SlotIndex);
	}
}

bool UMFBackpackWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// 声明本区域可接受 drop（否则拖回背包内也会被判为 cancelled 而误丢）。
	return true;
}

bool UMFBackpackWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// 拖回背包内松开 → 吸收（放回原位，不丢）。格子交换后置。
	return true;
}

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFBackpackWidget.generated.h"

class UPanelWidget;
class UMFItemSlotWidget;
class UMFInventoryComponent;

/**
 * 背包格子容器。仿 UMFMainHUDWidget 的 PetSlot 部分：逻辑全在 C++。
 *
 * **固定画满 N 格（含空位）**，首次 BuildSlots 建 N 个格子缓存，
 * 之后订阅 OnInventoryChanged 逐格 RefreshSlots（不重建，平滑刷新）。
 *
 * Usage:
 *   1. 建 WBP_Backpack 继承本类。
 *   2. Designer 放布局容器命名 "SlotGrid"：VerticalBox（单列紧凑）/ WrapBox（自动多列）/
 *      HorizontalBox 均可——用 AddChild 顺序填入，排布由容器决定。
 *   3. Details 设 ItemSlotClass=WBP_ItemSlot。
 *   4. WBP_MainHUD 放 WBP_Backpack 命名 "BackpackWidget"。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFBackpackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 绑定背包组件：按容量首建 N 格 + 订阅刷新。由 UMFMainHUDWidget::InitPlayerHUD 调用。 */
	void InitBackpack(UMFInventoryComponent* Inv);

	/** 格子容器（VerticalBox / WrapBox / HorizontalBox 等）。Designer 命名 "SlotGrid"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotGrid;

	/** 格子 Widget 类（继承 UMFItemSlotWidget）。WBP_Backpack Details 赋值。 */
	UPROPERTY(EditDefaultsOnly, Category = "Backpack")
	TSubclassOf<UMFItemSlotWidget> ItemSlotClass;

protected:
	virtual void NativeDestruct() override;

	// 背包区域接受 drop：拖回背包内松开 → 吸收（放回原位，不丢）；
	// 只有拖到区域外无 target 才走格子的 OnDragCancelled 丢弃。
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	/** 首次按容量建 N 格（含空位），缓存进 SlotWidgets。 */
	void BuildSlots(int32 NumSlots);

	/** 逐格 SetSlot 填充（不重建）。 */
	void RefreshSlots();

	UFUNCTION() void HandleInventoryChanged();
	UFUNCTION() void HandleSlotDiscard(int32 SlotIndex);
	UFUNCTION() void HandleSlotUse(int32 SlotIndex);

	UPROPERTY() TWeakObjectPtr<UMFInventoryComponent> BoundInventory;
	UPROPERTY() TArray<TObjectPtr<UMFItemSlotWidget>> SlotWidgets;
};

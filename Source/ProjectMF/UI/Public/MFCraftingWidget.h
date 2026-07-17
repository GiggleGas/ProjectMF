// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MFCraftingWidget.generated.h"

class UPanelWidget;
class UMFCraftRowWidget;
class UMFCraftingComponent;

/**
 * 合成页。仿 UMFBackpackWidget：逻辑全 C++。
 *
 * 列出全部配方（一行一个 UMFCraftRowWidget），订阅 OnCraftableChanged 逐行刷新可合成态；
 * 点击可合成的行 → Craft。
 *
 * Usage:
 *   1. 建 WBP_Crafting 继承本类，放布局容器命名 "RecipeList"，Details 设 RecipeRowClass。
 *   2. WBP_MainHUD 放 WBP_Crafting 命名 "CraftingWidget"。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTMF_API UMFCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 绑定打造组件：建行 + 订阅刷新。由 UMFMainHUDWidget::InitPlayerHUD 调用。 */
	void InitCrafting(UMFCraftingComponent* Crafting);

	/** 配方行容器（VerticalBox 等）。Designer 命名 "RecipeList"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RecipeList;

	/** 配方行 Widget 类（继承 UMFCraftRowWidget）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UMFCraftRowWidget> RecipeRowClass;

protected:
	virtual void NativeDestruct() override;

private:
	void BuildRows();
	void RefreshRows();
	UFUNCTION() void HandleCraftableChanged();
	UFUNCTION() void HandleCraftClicked(FName RecipeID);

	UPROPERTY() TWeakObjectPtr<UMFCraftingComponent> BoundCrafting;
	UPROPERTY() TArray<TObjectPtr<UMFCraftRowWidget>> RowWidgets;
};

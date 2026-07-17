// Copyright ProjectMF. All Rights Reserved.

#include "MFCraftingWidget.h"
#include "MFCraftRowWidget.h"
#include "MFCraftingComponent.h"
#include "Components/PanelWidget.h"

void UMFCraftingWidget::InitCrafting(UMFCraftingComponent* Crafting)
{
	if (!Crafting) return;

	BoundCrafting = Crafting;
	Crafting->OnCraftableChanged.AddDynamic(this, &UMFCraftingWidget::HandleCraftableChanged);

	BuildRows();
	RefreshRows();
}

void UMFCraftingWidget::NativeDestruct()
{
	if (UMFCraftingComponent* Crafting = BoundCrafting.Get())
	{
		Crafting->OnCraftableChanged.RemoveDynamic(this, &UMFCraftingWidget::HandleCraftableChanged);
	}
	Super::NativeDestruct();
}

void UMFCraftingWidget::BuildRows()
{
	if (!RecipeList || !RecipeRowClass) return;

	UMFCraftingComponent* Crafting = BoundCrafting.Get();
	if (!Crafting) return;

	RecipeList->ClearChildren();
	RowWidgets.Reset();

	for (const FName& RecipeID : Crafting->GetAllRecipes())
	{
		UMFCraftRowWidget* Row = CreateWidget<UMFCraftRowWidget>(this, RecipeRowClass);
		if (!Row) continue;

		Row->OnCraftClicked.AddDynamic(this, &UMFCraftingWidget::HandleCraftClicked);
		Row->SetRecipe(RecipeID, Crafting->GetRecipeDisplayName(RecipeID), Crafting->CanCraft(RecipeID));
		RecipeList->AddChild(Row);
		RowWidgets.Add(Row);
	}
}

void UMFCraftingWidget::RefreshRows()
{
	UMFCraftingComponent* Crafting = BoundCrafting.Get();
	if (!Crafting) return;

	for (UMFCraftRowWidget* Row : RowWidgets)
	{
		if (!Row) continue;
		const FName RecipeID = Row->GetRecipeID();
		Row->SetRecipe(RecipeID, Crafting->GetRecipeDisplayName(RecipeID), Crafting->CanCraft(RecipeID));
	}
}

void UMFCraftingWidget::HandleCraftableChanged()
{
	RefreshRows();
}

void UMFCraftingWidget::HandleCraftClicked(FName RecipeID)
{
	if (UMFCraftingComponent* Crafting = BoundCrafting.Get())
	{
		Crafting->Craft(RecipeID);   // Craft → 背包变化 → OnCraftableChanged → RefreshRows 自动
	}
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFCraftRowWidget.h"
#include "Components/TextBlock.h"

void UMFCraftRowWidget::SetRecipe(FName InRecipeID, const FText& InDisplayName, bool bInCanCraft)
{
	RecipeID  = InRecipeID;
	bCanCraft = bInCanCraft;

	if (RecipeName)
	{
		RecipeName->SetText(InDisplayName);
	}
	OnCraftableStateChanged(bCanCraft);
}

FReply UMFCraftRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bCanCraft && !RecipeID.IsNone())
	{
		OnCraftClicked.Broadcast(RecipeID);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// Copyright ProjectMF. All Rights Reserved.

#include "MFCraftingComponent.h"
#include "MFCraftingSettings.h"
#include "MFRecipeTypes.h"
#include "MFInventoryComponent.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"
#include "MFItemTypes.h"
#include "MFLog.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UMFCraftingComponent::UMFCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMFCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	// 背包变化 → 转发 OnCraftableChanged（UI 据此刷新可合成高亮）。
	if (UMFInventoryComponent* Inv = GetInventory())
	{
		Inv->OnInventoryChanged.AddDynamic(this, &UMFCraftingComponent::HandleInventoryChanged);
	}
}

UMFInventoryComponent* UMFCraftingComponent::GetInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UMFInventoryComponent>() : nullptr;
}

const FMFRecipeDef* UMFCraftingComponent::FindRecipe(FName RecipeID) const
{
	const UDataTable* Table = UMFCraftingSettings::GetRecipeTable();
	if (!Table) return nullptr;
	return Table->FindRow<FMFRecipeDef>(RecipeID, TEXT("MFCraftingComponent::FindRecipe"), false);
}

TArray<FName> UMFCraftingComponent::GetAllRecipes() const
{
	const UDataTable* Table = UMFCraftingSettings::GetRecipeTable();
	return Table ? Table->GetRowNames() : TArray<FName>();
}

bool UMFCraftingComponent::CanCraft(FName RecipeID) const
{
	const FMFRecipeDef* Recipe = FindRecipe(RecipeID);
	const UMFInventoryComponent* Inv = GetInventory();
	if (!Recipe || !Inv) return false;

	if (Recipe->Output.ItemID <= 0) return false;

	for (const FMFItemCount& In : Recipe->Inputs)
	{
		if (In.Item.ItemID <= 0 || In.Count <= 0) return false;
		if (!Inv->HasResource(In.Item.ItemID, In.Count)) return false;
	}
	return true;
}

bool UMFCraftingComponent::Craft(FName RecipeID)
{
	if (!CanCraft(RecipeID))
	{
		MF_LOG_WARNING(LogMFCraft, TEXT("Craft: 无法合成 %s（料不足或配方无效）。"), *RecipeID.ToString());
		return false;
	}

	const FMFRecipeDef* Recipe = FindRecipe(RecipeID);
	UMFInventoryComponent* Inv = GetInventory();

	// 扣料。
	for (const FMFItemCount& In : Recipe->Inputs)
	{
		Inv->RemoveResource(In.Item.ItemID, In.Count);
	}

	// 产出入包。
	const int32 Added = Inv->AddResource(Recipe->Output.ItemID, Recipe->OutputCount);
	MF_LOG(LogMFCraft, TEXT("Craft: %s → 产出 #%d x%d（入包 %d）。"),
		*RecipeID.ToString(), Recipe->Output.ItemID, Recipe->OutputCount, Added);
	return true;
}

FText UMFCraftingComponent::GetRecipeDisplayName(FName RecipeID) const
{
	const FMFRecipeDef* Recipe = FindRecipe(RecipeID);
	if (!Recipe) return FText::FromName(RecipeID);

	if (!Recipe->DisplayName.IsEmpty()) return Recipe->DisplayName;

	// 回退产出物 DisplayName。
	if (const FMFItemDef* Out = UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), Recipe->Output.ItemID))
	{
		if (!Out->DisplayName.IsEmpty()) return Out->DisplayName;
	}
	return FText::FromName(RecipeID);
}

void UMFCraftingComponent::HandleInventoryChanged()
{
	OnCraftableChanged.Broadcast();
}

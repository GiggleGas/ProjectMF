// Copyright ProjectMF. All Rights Reserved.

#include "MFCraftingSettings.h"
#include "Engine/DataTable.h"

const UDataTable* UMFCraftingSettings::GetRecipeTable()
{
	return GetDefault<UMFCraftingSettings>()->RecipeLibrary.LoadSynchronous();
}

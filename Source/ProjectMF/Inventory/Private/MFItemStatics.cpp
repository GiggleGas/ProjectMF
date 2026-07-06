// Copyright ProjectMF. All Rights Reserved.

#include "MFItemStatics.h"
#include "Engine/DataTable.h"

FName UMFItemStatics::ItemIDToRowName(int32 ItemID)
{
	return FName(*FString::FromInt(ItemID));
}

int32 UMFItemStatics::RowNameToItemID(FName RowName)
{
	return FCString::Atoi(*RowName.ToString());
}

const FMFItemDef* UMFItemStatics::FindItem(const UDataTable* Table, int32 ItemID)
{
	if (!Table || ItemID <= 0)
	{
		return nullptr;
	}
	static const FString Ctx(TEXT("MFItemStatics::FindItem"));
	return Table->FindRow<FMFItemDef>(ItemIDToRowName(ItemID), Ctx, /*bWarnIfRowMissing*/ false);
}

bool UMFItemStatics::GetItem(const UDataTable* Table, int32 ItemID, FMFItemDef& OutDef)
{
	if (const FMFItemDef* Found = FindItem(Table, ItemID))
	{
		OutDef = *Found;
		return true;
	}
	return false;
}

bool UMFItemStatics::ContainsItem(const UDataTable* Table, int32 ItemID)
{
	return FindItem(Table, ItemID) != nullptr;
}

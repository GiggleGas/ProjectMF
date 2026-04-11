// Copyright ProjectMF. All Rights Reserved.

#include "MFItemDatabase.h"
#include "MFLog.h"

// ============================================================
// PostLoad / Cache
// ============================================================

void UMFItemDatabase::PostLoad()
{
	Super::PostLoad();
	BuildLookupCache();
}

void UMFItemDatabase::BuildLookupCache()
{
	LookupCache.Empty(Items.Num());

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FName& ID = Items[i].ItemID;

		if (ID.IsNone())
		{
			MF_LOG_WARNING(LogMFInventory,
				TEXT("UMFItemDatabase: Item[%d] has an empty ItemID — skipped."), i);
			continue;
		}

		if (LookupCache.Contains(ID))
		{
			MF_LOG_WARNING(LogMFInventory,
				TEXT("UMFItemDatabase: Duplicate ItemID '%s' at index %d — later entry ignored."),
				*ID.ToString(), i);
			continue;
		}

		LookupCache.Add(ID, i);
	}

	MF_LOG(LogMFInventory, TEXT("UMFItemDatabase: Cached %d / %d items."),
		LookupCache.Num(), Items.Num());
}

// ============================================================
// 查询接口
// ============================================================

const FMFItemDef* UMFItemDatabase::FindItem(FName ItemID) const
{
	if (const int32* Idx = LookupCache.Find(ItemID))
	{
		return &Items[*Idx];
	}
	return nullptr;
}

bool UMFItemDatabase::GetItem(FName ItemID, FMFItemDef& OutDef) const
{
	if (const FMFItemDef* Found = FindItem(ItemID))
	{
		OutDef = *Found;
		return true;
	}
	return false;
}

bool UMFItemDatabase::ContainsItem(FName ItemID) const
{
	return LookupCache.Contains(ItemID);
}

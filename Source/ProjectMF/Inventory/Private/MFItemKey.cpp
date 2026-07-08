// Copyright ProjectMF. All Rights Reserved.

#include "MFItemKey.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"

const FMFItemDef* FMFItemKey::Resolve() const
{
	return UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID);
}

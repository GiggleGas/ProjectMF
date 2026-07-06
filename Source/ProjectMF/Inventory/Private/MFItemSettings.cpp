// Copyright ProjectMF. All Rights Reserved.

#include "MFItemSettings.h"
#include "Engine/DataTable.h"

const UDataTable* UMFItemSettings::GetItemTable()
{
	return GetDefault<UMFItemSettings>()->ItemDatabase.LoadSynchronous();
}

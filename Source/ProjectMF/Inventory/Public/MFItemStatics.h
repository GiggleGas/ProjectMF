// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MFItemTypes.h"
#include "MFItemStatics.generated.h"

class UDataTable;

/**
 * UMFItemStatics — 物品总表（DT_ItemDatabase）查询库。
 *
 * 物品总表 = 一张 DataTable，行结构 FMFItemDef，RowName 用数字字符串（"1001"…），
 * 数字即 int32 ItemID。所有系统用 int32 ItemID 索引，经本库转成 RowName 查表。
 */
UCLASS()
class PROJECTMF_API UMFItemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 数字 ItemID → 总表 RowName（数字字符串）。 */
	static FName  ItemIDToRowName(int32 ItemID);
	/** RowName（数字字符串）→ 数字 ItemID；非数字 RowName 返回 0。 */
	static int32  RowNameToItemID(FName RowName);

	/** 按数字 ItemID 从总表查行。表空 / ItemID<=0 / 无此行返回 nullptr。C++ 专用。 */
	static const FMFItemDef* FindItem(const UDataTable* Table, int32 ItemID);

	/** Blueprint 版：查到填 OutDef 返回 true。 */
	UFUNCTION(BlueprintPure, Category = "Item")
	static bool GetItem(const UDataTable* Table, int32 ItemID, FMFItemDef& OutDef);

	/** 该 ItemID 是否存在于总表。 */
	UFUNCTION(BlueprintPure, Category = "Item")
	static bool ContainsItem(const UDataTable* Table, int32 ItemID);
};

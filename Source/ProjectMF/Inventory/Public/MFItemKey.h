// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFItemKey.generated.h"

struct FMFItemDef;

/**
 * FMFItemKey — 物品引用（对物品总表 DT_Item 一行的强类型引用）。
 *
 * 数据上等价 int32 ItemID（存档/序列化零负担）；语义上是"物品引用"，带 Resolve()。
 * 编辑器里由 FMFItemKeyCustomization 画成下拉框（列 DT_Item 所有物品，点选而非手输数字）。
 *
 * 用在编辑器配置面（掉落表 Entry / 配方 Inputs 等）；运行时数据/调试入口仍可用裸 int32。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFItemKey
{
	GENERATED_BODY()

	/** 物品数字 ID（= 物品总表 DataTable 的 RowName）。编辑器下拉选择。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemID = 0;

	/** 从物品总表查定义；找不到返回 nullptr。 */
	const FMFItemDef* Resolve() const;

	bool IsValid() const { return ItemID > 0; }
	bool operator==(const FMFItemKey& Other) const { return ItemID == Other.ItemID; }
	friend uint32 GetTypeHash(const FMFItemKey& Key) { return ::GetTypeHash(Key.ItemID); }
};

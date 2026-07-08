// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Types/SlateEnums.h"

class IPropertyHandle;
class SWidget;

/**
 * FMFItemKey 的属性下拉控件：把物品引用画成一个下拉框，
 * 选项 = DT_Item 每行的 "物品名 (#ID)"，选中写回内部 ItemID。
 */
class FMFItemKeyCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> ItemIDHandle;

	/** 下拉选项（显示字符串）+ 显示串 → ItemID 反查。 */
	TArray<TSharedPtr<FString>> Options;
	TMap<FString, int32> DisplayToID;

	void BuildOptions();
	FText GetCurrentDisplayText() const;
	void OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<FString> InItem);
};

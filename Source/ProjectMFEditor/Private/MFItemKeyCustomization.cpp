// Copyright ProjectMF. All Rights Reserved.

#include "MFItemKeyCustomization.h"
#include "MFItemKey.h"
#include "MFItemTypes.h"
#include "MFItemSettings.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Engine/DataTable.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MFItemKeyCustomization"

TSharedRef<IPropertyTypeCustomization> FMFItemKeyCustomization::MakeInstance()
{
	return MakeShared<FMFItemKeyCustomization>();
}

void FMFItemKeyCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ItemIDHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMFItemKey, ItemID));

	BuildOptions();

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.f)
	[
		SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&Options)
		.OnGenerateWidget(this, &FMFItemKeyCustomization::OnGenerateComboWidget)
		.OnSelectionChanged(this, &FMFItemKeyCustomization::OnSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FMFItemKeyCustomization::GetCurrentDisplayText)
		]
	];
}

void FMFItemKeyCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> /*PropertyHandle*/, IDetailChildrenBuilder& /*ChildBuilder*/, IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	// header 一行下拉够用，不展开子项（也不暴露裸 ItemID）。
}

void FMFItemKeyCustomization::BuildOptions()
{
	Options.Reset();
	DisplayToID.Reset();

	const UDataTable* Table = UMFItemSettings::GetItemTable();
	if (!Table) return;

	Table->ForeachRow<FMFItemDef>(TEXT("MFItemKeyCustomization::BuildOptions"),
		[this](const FName& RowName, const FMFItemDef& Row)
		{
			const int32 ItemID = FCString::Atoi(*RowName.ToString());
			const FString Display = FString::Printf(TEXT("%s (#%d)"), *Row.DisplayName.ToString(), ItemID);
			Options.Add(MakeShared<FString>(Display));
			DisplayToID.Add(Display, ItemID);
		});
}

FText FMFItemKeyCustomization::GetCurrentDisplayText() const
{
	if (!ItemIDHandle.IsValid()) return FText::GetEmpty();

	int32 Cur = 0;
	ItemIDHandle->GetValue(Cur);
	if (Cur <= 0) return LOCTEXT("None", "(未选择)");

	for (const TPair<FString, int32>& Pair : DisplayToID)
	{
		if (Pair.Value == Cur) return FText::FromString(Pair.Key);
	}
	return FText::FromString(FString::Printf(TEXT("#%d (缺失)"), Cur));
}

void FMFItemKeyCustomization::OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type /*SelectInfo*/)
{
	if (!NewSelection.IsValid() || !ItemIDHandle.IsValid()) return;
	if (const int32* ID = DisplayToID.Find(*NewSelection))
	{
		ItemIDHandle->SetValue(*ID);
	}
}

TSharedRef<SWidget> FMFItemKeyCustomization::OnGenerateComboWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
}

#undef LOCTEXT_NAMESPACE

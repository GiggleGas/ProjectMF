// Copyright ProjectMF. All Rights Reserved.

#include "ProjectMF/UI/Public/MFOverheadWidget.h"

#include "AbilitySystemComponent.h"
#include "MFAttributeSetBase.h"

void UMFOverheadWidget::InitWithASC(UAbilitySystemComponent* InASC)
{
	if (!InASC) return;
	BoundASC = InASC;

	// Push current values immediately so the bar is correct on first frame.
	bool bFound;
	const float CurHP = InASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetHealthAttribute(), bFound);
	const float MaxHP = InASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMaxHealthAttribute(), bFound);
	BP_OnHealthChanged(CurHP, MaxHP);

	HealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(
		UMFAttributeSetBase::GetHealthAttribute())
		.AddUObject(this, &UMFOverheadWidget::OnHealthAttributeChanged);

	MaxHealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(
		UMFAttributeSetBase::GetMaxHealthAttribute())
		.AddUObject(this, &UMFOverheadWidget::OnMaxHealthAttributeChanged);
}

void UMFOverheadWidget::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		bool bFound;
		const float MaxHP = ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMaxHealthAttribute(), bFound);
		BP_OnHealthChanged(Data.NewValue, MaxHP);
	}
}

void UMFOverheadWidget::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		bool bFound;
		const float CurHP = ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetHealthAttribute(), bFound);
		BP_OnHealthChanged(CurHP, Data.NewValue);
	}
}

void UMFOverheadWidget::NativeDestruct()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMFAttributeSetBase::GetHealthAttribute())
			.Remove(HealthChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UMFAttributeSetBase::GetMaxHealthAttribute())
			.Remove(MaxHealthChangedHandle);
	}
	Super::NativeDestruct();
}

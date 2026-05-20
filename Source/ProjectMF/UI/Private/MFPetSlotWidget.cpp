// Copyright ProjectMF. All Rights Reserved.

#include "MFPetSlotWidget.h"
#include "MFOverheadWidget.h"
#include "MFPetBase.h"
#include "MFItemDatabase.h"
#include "MFItemTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/Image.h"

void UMFPetSlotWidget::InitWithPetActor(AMFPetBase* Pet, UMFItemDatabase* Database)
{
	if (!Pet) return;

	// --- 头像 ---
	if (Portrait && Database)
	{
		if (const FMFItemDef* Def = Database->FindItem(Pet->PetItemID))
		{
			if (Def->Icon)
			{
				Portrait->SetBrushFromTexture(Def->Icon);
			}
		}
	}

	// --- 血条 ---
	if (PetHPBar)
	{
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pet))
		{
			PetHPBar->InitWithASC(ASCInterface->GetAbilitySystemComponent());
		}
	}
}

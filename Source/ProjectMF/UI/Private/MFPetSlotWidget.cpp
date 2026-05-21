// Copyright ProjectMF. All Rights Reserved.

#include "MFPetSlotWidget.h"
#include "MFOverheadWidget.h"
#include "MFPetBase.h"
#include "MFPetConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/Image.h"

void UMFPetSlotWidget::InitWithPetActor(AMFPetBase* Pet)
{
	if (!Pet) return;

	// --- 头像：从 CachedPetConfig 取 Icon（软引用，LoadSynchronous 同步加载）---
	if (Portrait)
	{
		if (const UMFPetConfig* Config = Pet->GetCachedPetConfig())
		{
			if (UTexture2D* Icon = Config->Icon.LoadSynchronous())
			{
				Portrait->SetBrushFromTexture(Icon);
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

// Copyright ProjectMF. All Rights Reserved.

#include "MFPetSlotWidget.h"
#include "MFOverheadWidget.h"
#include "MFPetBase.h"
#include "MFPetConfig.h"
#include "MFAIRegistry.h"
#include "MFItemTypes.h"
#include "AbilitySystemInterface.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"

namespace
{
	constexpr float ReviveDimAlpha = 0.4f;   // 复活读秒时头像变暗的灰度
}

void UMFPetSlotWidget::InitWithPet(const FMFPetInstance& Inst, AMFPetBase* LiveActor, UDataTable* AIRegistry)
{
	// --- 头像：出战时取 Actor 的 CachedConfig，否则按 AIConfigID 查注册表 ---
	const UMFPetConfig* Config = nullptr;
	if (LiveActor)
	{
		Config = LiveActor->GetCachedPetConfig();
	}
	else if (AIRegistry)
	{
		if (const FMFAIRegistryRow* Row = AIRegistry->FindRow<FMFAIRegistryRow>(Inst.AIConfigID, TEXT("PetSlot")))
		{
			Config = Row->Config.LoadSynchronous();
		}
	}
	ApplyPortrait(Config);

	// --- 血条：仅出战中绑定并显示，否则隐藏 ---
	if (PetHPBar)
	{
		if (LiveActor)
		{
			if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(LiveActor))
			{
				PetHPBar->InitWithASC(ASCInterface->GetAbilitySystemComponent());
			}
			PetHPBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PetHPBar->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// --- 复活读秒 + 头像变暗 ---
	if (Inst.bIsDead)
	{
		if (Portrait)
		{
			Portrait->SetColorAndOpacity(FLinearColor(ReviveDimAlpha, ReviveDimAlpha, ReviveDimAlpha, 1.f));
		}
		UpdateReviveRemaining(Inst.ReviveTimeRemaining);
	}
	else
	{
		if (Portrait)
		{
			Portrait->SetColorAndOpacity(FLinearColor::White);
		}
		if (ReviveCountdownText)
		{
			ReviveCountdownText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UMFPetSlotWidget::UpdateReviveRemaining(float SecondsRemaining)
{
	if (!ReviveCountdownText) return;

	const int32 Secs = FMath::Max(0, FMath::CeilToInt(SecondsRemaining));
	ReviveCountdownText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Secs)));
	ReviveCountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMFPetSlotWidget::ApplyPortrait(const UMFPetConfig* Config)
{
	if (!Portrait || !Config) return;

	if (UTexture2D* Icon = Config->Icon.LoadSynchronous())
	{
		Portrait->SetBrushFromTexture(Icon);
	}
}

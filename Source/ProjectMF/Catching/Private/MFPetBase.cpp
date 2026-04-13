// Copyright ProjectMF. All Rights Reserved.

#include "MFPetBase.h"
#include "MFItemTypes.h"
#include "MFAttributeSetBase.h"
#include "MFLog.h"
#include "AbilitySystemComponent.h"

// ============================================================
// 构造
// ============================================================

AMFPetBase::AMFPetBase()
{
	// AMFCharacterBase 的 Tick 已足够，宠物不需要额外的 ActorTick 逻辑
	// 具体 AI 行为由 Behavior Tree / Mass 驱动
}

// ============================================================
// IMFCatchable 实现
// ============================================================

bool AMFPetBase::CanBeCaught_Implementation(const AActor* Catcher) const
{
	if (bIsCaught)
	{
		MF_LOG_WARNING(LogMFCatch,
			TEXT("AMFPetBase::CanBeCaught — %s is already caught, returning false."),
			*GetName());
		return false;
	}

	MF_LOG(LogMFCatch,
		TEXT("AMFPetBase::CanBeCaught — %s is available to be caught by %s."),
		*GetName(),
		Catcher ? *Catcher->GetName() : TEXT("Unknown"));

	return true;
}

void AMFPetBase::OnCaught_Implementation(AActor* Catcher)
{
	bIsCaught = true;

	MF_LOG(LogMFCatch,
		TEXT("AMFPetBase::OnCaught — %s has been caught by %s."),
		*GetName(),
		Catcher ? *Catcher->GetName() : TEXT("Unknown"));

	// Actor 的销毁由调用方（GA_CatchPet::EndCatch）在序列化完成后负责。
	// 子类在 Super:: 之后可播放收服动画/粒子，动效结束前不要提前 Destroy。
}

// ============================================================
// 序列化
// ============================================================

void AMFPetBase::SerializeToInstance(FMFPetInstance& InOutInstance) const
{
	InOutInstance.PetItemID = PetItemID;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		MF_LOG_WARNING(LogMFCatch,
			TEXT("AMFPetBase::SerializeToInstance — ASC not found on %s, AttributeSnapshot will be empty."),
			*GetName());
		return;
	}

	bool bFound = false;
	InOutInstance.AttributeSnapshot.Add(TEXT("Health"),
		ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetHealthAttribute(), bFound));
	InOutInstance.AttributeSnapshot.Add(TEXT("MaxHealth"),
		ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMaxHealthAttribute(), bFound));
	InOutInstance.AttributeSnapshot.Add(TEXT("MoveSpeed"),
		ASC->GetGameplayAttributeValue(UMFAttributeSetBase::GetMoveSpeedAttribute(), bFound));
}

void AMFPetBase::RestoreFromInstance(const FMFPetInstance& Instance)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		MF_LOG_WARNING(LogMFCatch,
			TEXT("AMFPetBase::RestoreFromInstance — ASC not found on %s, attributes not restored."),
			*GetName());
		return;
	}

	auto Restore = [&](const TCHAR* Key, const FGameplayAttribute& Attr)
	{
		if (const float* Val = Instance.AttributeSnapshot.Find(Key))
		{
			ASC->SetNumericAttributeBase(Attr, *Val);
		}
	};

	Restore(TEXT("Health"),    UMFAttributeSetBase::GetHealthAttribute());
	Restore(TEXT("MaxHealth"), UMFAttributeSetBase::GetMaxHealthAttribute());
	Restore(TEXT("MoveSpeed"), UMFAttributeSetBase::GetMoveSpeedAttribute());
}

void AMFPetBase::OnCatchFailed_Implementation(AActor* Catcher)
{
	MF_LOG_WARNING(LogMFCatch,
		TEXT("AMFPetBase::OnCatchFailed — %s escaped from %s."),
		*GetName(),
		Catcher ? *Catcher->GetName() : TEXT("Unknown"));

	// TODO: 切换到逃跑 Behavior Tree 节点或触发逃跑 GE/Tag
	// 子类在 Super::OnCatchFailed_Implementation(Catcher) 之后添加具体逻辑
}

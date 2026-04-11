// Copyright ProjectMF. All Rights Reserved.

#include "MFPetBase.h"
#include "MFLog.h"

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

	// TODO: 在销毁前将宠物数据注册到玩家背包（库存系统待实现）
	// TODO: 可在 Destroy 前播放收服动画 / 粒子（子类重写，在效果结束后再调用 Destroy）

	// 从世界中移除宠物 Actor
	Destroy();
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

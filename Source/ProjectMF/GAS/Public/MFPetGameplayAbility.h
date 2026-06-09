// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "MFPetGameplayAbility.generated.h"

/**
 * AI 战斗技能的归属基类（拥有者轴）。
 *
 * 涵盖所有由 AI 控制的战斗者——宠物、敌人、Boss——释放的技能
 * （近战 / 远程 / 移动技能 等）。后续在此集中战斗技能的激活门禁
 * （死亡 / 眩晕 / 未召唤 / 逃跑中 等）——
 * 当前为纯骨架，门禁逻辑待策划案更新后再补（见任务 A3）。
 */
UCLASS(Abstract)
class PROJECTMF_API UMFPetGameplayAbility : public UMFGameplayAbilityBase
{
	GENERATED_BODY()
};

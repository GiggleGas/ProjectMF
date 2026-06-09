// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "MFPlayerGameplayAbility.generated.h"

/**
 * 玩家技能的归属基类（拥有者轴）。
 *
 * 所有由玩家直接释放的技能（采集 / 抓宠 / 召唤 等）继承此类。
 * 后续在此集中玩家专有的激活门禁（输入态 / UI 态 / 死亡 等）——
 * 当前为纯骨架，门禁逻辑待策划案更新后再补（见任务 A3）。
 */
UCLASS(Abstract)
class PROJECTMF_API UMFPlayerGameplayAbility : public UMFGameplayAbilityBase
{
	GENERATED_BODY()
};

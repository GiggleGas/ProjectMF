// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "STCond_IsBeyondWander.generated.h"

/**
 * InstanceData：无输入（读 pawn 上的 UMFHomeAnchorComponent）。
 */
USTRUCT()
struct PROJECTMF_API FSTCond_IsBeyondWander_InstanceData
{
	GENERATED_BODY()
};

/**
 * STCond_IsBeyondWander — AI 是否离出生锚点超过游离半径（该回家）。
 *
 * 读 pawn 上的 UMFHomeAnchorComponent，返回 IsBeyondWander()。
 * 典型用法（StateTree 编辑器）：ReturnHome 状态的进入条件 = 无目标 + 本条件为真。
 *   （无目标用 STCond_OwnerHasTag(MF.AI.Perception.HasTarget) + bInvert）
 * 见 Docs/HomeAnchor_Leash_Design.md。
 */
USTRUCT(DisplayName = "MF Is Beyond Wander")
struct PROJECTMF_API FSTCond_IsBeyondWander : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTCond_IsBeyondWander_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

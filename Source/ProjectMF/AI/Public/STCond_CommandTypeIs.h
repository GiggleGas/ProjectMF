// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "MFCommandTypes.h"
#include "STCond_CommandTypeIs.generated.h"

/**
 * InstanceData：
 *   ExpectedType — 期望的指令类型（在 StateTree 编辑器中配置）。
 */
USTRUCT()
struct PROJECTMF_API FSTCond_CommandTypeIs_InstanceData
{
	GENERATED_BODY()

	/** 期望的指令类型。当宠物有待执行指令且其类型 == 此值时条件成立。 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	EMFCommandType ExpectedType = EMFCommandType::MoveTo;
};

/**
 * STCond_CommandTypeIs — 检测宠物当前待执行指令是否为指定类型。
 *
 * 用于 PlayerCommand 子树中 Move / Skill 各分支的进入条件，决定走哪条执行分支。
 * 读取受控 Pawn 上的 UMFPetCommandComponent；无指令或类型不符则为 false。
 */
USTRUCT(DisplayName = "MF Command Type Is")
struct PROJECTMF_API FSTCond_CommandTypeIs : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTCond_CommandTypeIs_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

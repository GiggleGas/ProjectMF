// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_ReadCommandLocation.generated.h"

/**
 * InstanceData：
 *   ResultLocation — 输出：从待执行指令中读取的移动目标点。
 *                    在 StateTree 编辑器中绑定到 FVector 变量，供内置 MoveTo 使用。
 */
USTRUCT()
struct PROJECTMF_API FSTTask_ReadCommandLocation_InstanceData
{
	GENERATED_BODY()

	/** 命令的移动目标点（输出）。绑定到状态树变量，内置 MoveTo 读取该变量。 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FVector ResultLocation = FVector::ZeroVector;
};

/**
 * STTask_ReadCommandLocation — 取出宠物当前 MoveTo 指令的目标点并输出，随即消费指令。
 *
 * 与 wander 的 FindRandomNavPoint 同款一次性任务：EnterState 写出 ResultLocation 后立即
 * 返回 Succeeded，由同状态内的内置 MoveTo 任务（绑定同一变量）执行实际移动。
 * 取到数据后立刻 ConsumeCommand，使执行中到来的新指令能干净抢占。
 *
 * 使用方式（StateTree 编辑器）：
 *   PlayerCommand/Move 状态：本任务 + 内置 MoveTo；
 *   将 ResultLocation 绑定到变量（如 CommandMoveTarget），MoveTo 的目标绑同一变量。
 */
USTRUCT(DisplayName = "MF Read Command Location")
struct PROJECTMF_API FSTTask_ReadCommandLocation : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_ReadCommandLocation_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext&       Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

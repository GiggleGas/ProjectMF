// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_GetHomeLocation.generated.h"

/**
 * InstanceData：
 *   HomeLocation — 输出：AI 出生锚点坐标。绑定到 StateTree FVector 变量，供 MoveTo 使用。
 */
USTRUCT()
struct PROJECTMF_API FSTTask_GetHomeLocation_InstanceData
{
	GENERATED_BODY()

	/** 输出：锚点坐标。在 ST 编辑器绑定到 FVector 变量（如 HomeLoc），供 MoveTo。 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FVector HomeLocation = FVector::ZeroVector;
};

/**
 * STTask_GetHomeLocation — 从 pawn 的 UMFHomeAnchorComponent 取出生锚点坐标。
 *
 * EnterState 一次性读取并写入 HomeLocation，返回 Succeeded（无目标组件 → Failed）。
 * 典型用法：ReturnHome 状态里 GetHomeLocation → 绑定 HomeLoc → Move To (HomeLoc)。
 * 见 Docs/HomeAnchor_Leash_Design.md。
 */
USTRUCT(DisplayName = "MF Get Home Location")
struct PROJECTMF_API FSTTask_GetHomeLocation : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_GetHomeLocation_InstanceData;

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

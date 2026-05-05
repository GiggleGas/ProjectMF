// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_FindRandomNavPoint.generated.h"

/**
 * InstanceData：
 *   MinRadius      — 采样环形区域的最小半径（可绑定输入变量）
 *   MaxRadius      — 采样环形区域的最大半径（可绑定输入变量）
 *   MaxRetries     — NavMesh 投影失败时的最大重试次数
 *   ResultLocation — 输出：成功找到的 NavMesh 点坐标
 */
USTRUCT()
struct PROJECTMF_API FSTTask_FindRandomNavPoint_InstanceData
{
	GENERATED_BODY()

	/** 采样最小半径（cm），防止目标点太近 */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0"))
	float MinRadius = 200.f;

	/** 采样最大半径（cm） */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0"))
	float MaxRadius = 800.f;

	/** NavMesh 投影失败时的最大重试次数 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "1", ClampMax = "20"))
	int32 MaxRetries = 5;

	/**
	 * 命中的 NavMesh 点（输出）。
	 * 在 StateTree 编辑器中绑定到 FVector 变量，供移动 Task 使用。
	 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FVector ResultLocation = FVector::ZeroVector;
};

/**
 * STTask_FindRandomNavPoint — 在指定环形区域内随机采样一个可达的 NavMesh 点。
 *
 * 行为：
 *   EnterState — 在 [MinRadius, MaxRadius] 环形范围内均匀采样候选点，
 *                投影到 NavMesh；成功 → 写入 ResultLocation，返回 Succeeded；
 *                重试 MaxRetries 次均失败 → 返回 Failed。
 *   无 Tick    — 一次性任务，立即返回结果，不需要每帧轮询。
 *
 * 使用方式（StateTree 编辑器）：
 *   1. 在 Wander/Patrol 状态下添加此 Task。
 *   2. 将 ResultLocation 绑定到状态树级别的 FVector 变量（如 "WanderTarget"）。
 *   3. 后续移动 Task（如 Move To Location）绑定同一变量。
 *   4. MinRadius / MaxRadius 可以直接填写固定值，也可以绑定上层变量动态调整。
 */
USTRUCT(DisplayName = "MF Find Random Nav Point")
struct PROJECTMF_API FSTTask_FindRandomNavPoint : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_FindRandomNavPoint_InstanceData;

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

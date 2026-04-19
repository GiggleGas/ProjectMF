// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_GetThreatTarget.generated.h"

/**
 * InstanceData：
 *   TargetActor  — 输出绑定，将当前威胁目标写入 StateTree 变量。
 *                  在 StateTree 编辑器中将此属性绑定到一个 Actor 类型的变量，
 *                  后续节点（移动、攻击等）通过该变量引用目标。
 */
USTRUCT()
struct PROJECTMF_API FSTTask_GetThreatTarget_InstanceData
{
	GENERATED_BODY()

	/**
	 * 当前锁定的威胁目标（输出）。
	 * 每次 Tick 从 UMFThreatComponent 刷新。
	 * 在 StateTree 编辑器中将此绑定到共享的 Actor 变量，供其他 Task / Condition 使用。
	 */
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
};

/**
 * STTask_GetThreatTarget — 从 UMFThreatComponent 获取当前索敌目标。
 *
 * 行为：
 *   EnterState — 查询 ThreatComponent.GetCurrentTarget()；
 *                有目标 → 写入 TargetActor，返回 Running；
 *                无目标 → 返回 Failed（StateTree 可据此切换到 Idle 状态）。
 *   Tick       — 每帧刷新 TargetActor（目标可能在锁定期切换）；
 *                目标丢失 → 返回 Failed，触发状态切换。
 *   ExitState  — 清空 TargetActor，避免悬空引用。
 *
 * 使用方式（StateTree 编辑器）：
 *   1. 在 Combat 状态下添加此 Task。
 *   2. 将 TargetActor 绑定到状态树级别的 Actor 变量（如 "TargetEnemy"）。
 *   3. 后续 Task（移动、攻击）绑定同一变量即可获取目标。
 *   4. 用 MF.AI.Perception.HasTarget 标签做进入 Combat 状态的 Condition。
 */
USTRUCT(DisplayName = "MF Get Threat Target")
struct PROJECTMF_API FSTTask_GetThreatTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_GetThreatTarget_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext&       Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float                DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext&       Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	/** 从 AIController 的 Pawn 上获取 ThreatComponent。 */
	class UMFThreatComponent* GetThreatComp(FStateTreeExecutionContext& Context) const;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

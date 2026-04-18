// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "AIController.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_ActivateAttack.generated.h"

class UAbilitySystemComponent;

/**
 * StateTree Task 的实例数据。
 * 无需在 StateTree 编辑器中配置任何技能类；
 * 攻击技能由 AI 蓝图的 DefaultAbilities 授予，并在 AbilityTags 中包含 MF.Ability.Attack。
 */
USTRUCT()
struct PROJECTMF_API FSTTask_ActivateAttack_InstanceData
{
	GENERATED_BODY()

	// Runtime: 激活后缓存 Spec Handle，用于 ExitState 时精确取消
	FGameplayAbilitySpecHandle ActiveSpecHandle;
};

/**
 * StateTree Task：激活 AI 攻击技能（MF.Ability.Attack tag）并等待其结束。
 *
 * 流程：
 *   EnterState → 查找带 MF.Ability.Attack tag 的已授予技能 → TryActivateAbility → Running
 *   Tick       → 检测 MF.Character.State.Attacking tag
 *              → tag 消失（技能正常结束）→ Succeeded
 *   ExitState  → 若 tag 仍在（被状态机打断）→ 取消技能
 *
 * 使用方式：
 *   在 AI 角色蓝图的 DefaultAbilities 中添加攻击技能，
 *   并在技能蓝图的 AbilityTags 中包含 MF.Ability.Attack。
 *   StateTree 编辑器中无需额外配置。
 */
USTRUCT(DisplayName = "MF Activate Attack Ability")
struct PROJECTMF_API FSTTask_ActivateAttack : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_ActivateAttack_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext&        Context,
		const FStateTreeTransitionResult&  Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float                 DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext&        Context,
		const FStateTreeTransitionResult&  Transition) const override;

private:
	UAbilitySystemComponent* GetASC(FStateTreeExecutionContext& Context) const;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

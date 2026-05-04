// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "AIController.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_ActivateAttack.generated.h"

class UAbilitySystemComponent;

/**
 * StateTree Task 的实例数据。
 * 在 StateTree 编辑器中配置 AbilityTag 和 ActiveStateTag：
 *   近战攻击：AbilityTag = MF.Ability.Attack        ActiveStateTag = MF.Character.State.Attacking
 *   远程攻击：AbilityTag = MF.Ability.RangedAttack  ActiveStateTag = MF.Character.State.RangedAttacking
 */
USTRUCT()
struct PROJECTMF_API FSTTask_ActivateAttack_InstanceData
{
	GENERATED_BODY()

	/** 要激活的技能标签。查找 ASC 中第一个 AbilityTags 包含此 Tag 的已授予技能。 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Categories = "MF.Ability"))
	FGameplayTag AbilityTag;

	/** 技能激活期间持有的状态标签。消失即视为技能正常结束。 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Categories = "MF.Character.State"))
	FGameplayTag ActiveStateTag;

	// Runtime: 激活后缓存 Spec Handle，用于 ExitState 时精确取消
	FGameplayAbilitySpecHandle ActiveSpecHandle;
};

/**
 * StateTree Task：按配置的 AbilityTag 激活技能，监听 ActiveStateTag 等待结束。
 *
 * 流程：
 *   EnterState → 查找 AbilityTag 对应的已授予技能 → TryActivateAbility → Running
 *   Tick       → ASC 不再持有 ActiveStateTag（技能正常结束）→ Succeeded
 *   ExitState  → ASC 仍持有 ActiveStateTag（被状态机打断）→ 取消技能
 *
 * 使用方式：
 *   在技能蓝图的 AbilityTags 中添加对应 Tag，
 *   并在 StateTree 编辑器的节点属性中填写 AbilityTag 和 ActiveStateTag。
 */
USTRUCT(DisplayName = "MF Activate Ability By Tag")
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

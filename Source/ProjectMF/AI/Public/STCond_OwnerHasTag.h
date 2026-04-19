// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "STCond_OwnerHasTag.generated.h"

/**
 * InstanceData：
 *   TagToCheck — 要检测的 GameplayTag（在 StateTree 编辑器中配置）。
 */
USTRUCT()
struct PROJECTMF_API FSTCond_OwnerHasTag_InstanceData
{
	GENERATED_BODY()

	/**
	 * 要检测的 GameplayTag。
	 * 典型值：MF.AI.Perception.HasTarget、MF.Character.State.Dead 等。
	 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	FGameplayTag TagToCheck;
};

/**
 * STCond_OwnerHasTag — 检测 AI 自身 ASC 是否拥有指定 GameplayTag。
 *
 * 使用方式（StateTree 编辑器）：
 *   将此 Condition 添加到状态的进入条件或转换条件中。
 *   配置 TagToCheck，例如 MF.AI.Perception.HasTarget。
 *   勾选 StateTree 内置的 bInvert 可实现"不拥有该 Tag"的判断。
 *
 * 典型场景：
 *   Idle → Combat 转换：TagToCheck = MF.AI.Perception.HasTarget
 *   Combat → Idle 转换：TagToCheck = MF.AI.Perception.HasTarget + bInvert
 *   任意状态：TagToCheck = MF.Character.State.Dead
 */
USTRUCT(DisplayName = "MF Owner Has Tag")
struct PROJECTMF_API FSTCond_OwnerHasTag : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTCond_OwnerHasTag_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

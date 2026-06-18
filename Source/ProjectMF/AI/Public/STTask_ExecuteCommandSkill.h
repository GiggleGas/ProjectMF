// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_ExecuteCommandSkill.generated.h"

class UAbilitySystemComponent;
class UMFPetCommandComponent;

/**
 * InstanceData：
 *   ActiveSpecHandle — 运行时缓存：激活的技能 Spec，用于等待结束 / 打断时取消。
 */
USTRUCT()
struct PROJECTMF_API FSTTask_ExecuteCommandSkill_InstanceData
{
	GENERATED_BODY()

	FGameplayAbilitySpecHandle ActiveSpecHandle;
};

/**
 * STTask_ExecuteCommandSkill — 执行宠物当前 CastSkill 指令：按指令携带的 SkillTag
 * 找到已授予技能并激活，等待其结束。
 *
 * 与 STTask_ActivateAttack 的区别：AbilityTag 来自运行时指令（而非编辑器写死），
 * 且结束判定用 FGameplayAbilitySpec::IsActive()（无需为每个技能配 ActiveStateTag，
 * 对冲撞/跳跃/近战/远程等任意技能通用）。
 *
 * 流程：
 *   EnterState — 读 SkillTag 并消费指令 → 找 spec → TryActivateAbility → Running（失败 Failed）
 *   Tick       — spec 不再 IsActive → Succeeded
 *   ExitState  — 仍 active（被转换打断）→ CancelAbilityHandle
 */
USTRUCT(DisplayName = "MF Execute Command Skill")
struct PROJECTMF_API FSTTask_ExecuteCommandSkill : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_ExecuteCommandSkill_InstanceData;

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
		const float                 DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext&       Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	UAbilitySystemComponent* GetASC(FStateTreeExecutionContext& Context) const;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AIController.h"
#include "Tasks/StateTreeAITask.h"
#include "STTask_PlayAnimation.generated.h"

class UPaperZDAnimSequence;
class UPaperZDAnimInstance;

/**
 * StateTree Task 实例数据。
 * AnimSequence / PlayRate / bLoop / SlotName 在 StateTree 编辑器中赋值。
 */
USTRUCT()
struct PROJECTMF_API FSTTask_PlayAnimation_InstanceData
{
	GENERATED_BODY()

	/** 要播放的 PaperZD 动画序列，必须赋值。 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UPaperZDAnimSequence> AnimSequence;

	/** 播放速率，1.0 = 正常速度。 */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	/**
	 * 是否循环播放。
	 *   true  → 动画结束后立即重新播放，Task 保持 Running 直到状态机退出。
	 *   false → 动画播完后 Task 返回 Succeeded。
	 */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bLoop = false;

	/** PaperZD Slot 名称，通常保持默认即可。 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FName SlotName = FName("DefaultSlot");

	// Runtime
	float ElapsedTime  = 0.f;
	float AnimDuration = 0.f;
};

/**
 * StateTree Task：直接播放一段 PaperZD AnimSequence（Override），用于调试动画管线。
 *
 * 流程：
 *   EnterState → PlayAnimationOverride → Running
 *   Tick       → 计时；
 *                bLoop=false → 时间到 → Succeeded
 *                bLoop=true  → 时间到 → 重播 → Running
 *   ExitState  → 无需显式停止（新状态的动画会自然覆盖）
 *
 * 使用方式：
 *   在 StateTree 编辑器中添加此 Task 并赋值 AnimSequence。
 *   角色须继承 AMFCharacterBase（含 UPaperZDAnimationComponent）。
 */
USTRUCT(DisplayName = "MF Play PaperZD Animation")
struct PROJECTMF_API FSTTask_PlayAnimation : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTTask_PlayAnimation_InstanceData;

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
	UPaperZDAnimInstance* GetAnimInstance(FStateTreeExecutionContext& Context) const;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

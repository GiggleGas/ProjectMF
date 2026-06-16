// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AIMoveAbilityBase.h"
#include "GA_Jump.generated.h"

class UMFJumpData;

/**
 * 跳跃（Jump）——抛物线腾跃到锁定落点、落地 AOE 结算的移动技能。
 *
 * 公共流程（前摇 / 蓄力跟随 / 接管移动 / 后摇 / 中断恢复）由 UGA_AIMoveAbilityBase 提供，
 * 本类只实现弧线腾跃本身：BeginMovement 锁定落点 + 启动 JumpTick；
 * JumpTick 沿抛物线逐帧位移；t>=1 → DoLanding 在落点做球形 AOE → 收尾。
 *
 * 落点 = 起跳瞬间目标位置（不追踪），目标可走位躲避；伤害只在落地结算（非沿途）。
 *
 * StateTree 经 STTask_ActivateAttack 以 AbilityTag=MF.Ability.Pet.Move.Jump、
 * ActiveStateTag=MF.GameplayState.Jumping 激活并等待。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_Jump : public UGA_AIMoveAbilityBase
{
	GENERATED_BODY()

public:

	UGA_Jump();

	/** 跳跃参数（含继承自基类的前摇/跟随/后摇等）。须在 BP defaults 中赋值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump|Config")
	TObjectPtr<UMFJumpData> JumpData;

protected:

	virtual UMFMoveAbilityData* GetMoveData() const override;
	virtual FGameplayTag        GetActiveStateTag() const override;
	virtual void                BeginMovement() override;

private:

	void JumpTick();
	void DoLanding();

	FVector StartPos  = FVector::ZeroVector;
	FVector LandPos   = FVector::ZeroVector;
	float   Elapsed   = 0.f;

	/** 本次跳跃的实际抛物线峰高（cm）= MaxJumpHeight × (实际距离 / 最大距离)，BeginMovement 计算。 */
	float   ArcHeight = 0.f;
};

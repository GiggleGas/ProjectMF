// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AIMoveAbilityBase.h"
#include "GA_Charge.generated.h"

class UMFChargeData;

/**
 * 冲撞（Charge）——沿锁定方向高速直线位移、沿途结算的移动技能。
 *
 * 公共流程（前摇 / 蓄力跟随 / 接管移动 / 后摇 / 中断恢复）由 UGA_AIMoveAbilityBase 提供，
 * 本类只实现冲刺位移本身：BeginMovement 锁定起步重叠 + 启动 DashTick；
 * DashTick 每帧位移 + 撞墙检测 + 球形命中（可选冲刺转向）；命中/撞墙/达最远距离 → 收尾。
 *
 * StateTree 经 STTask_ActivateAttack 以 AbilityTag=MF.Ability.Pet.Move.Charge、
 * ActiveStateTag=MF.GameplayState.Charging 激活并等待。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_Charge : public UGA_AIMoveAbilityBase
{
	GENERATED_BODY()

public:

	UGA_Charge();

	/** 冲撞参数（含继承自基类的前摇/跟随/后摇等）。须在 BP defaults 中赋值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge|Config")
	TObjectPtr<UMFChargeData> ChargeData;

protected:

	virtual UMFMoveAbilityData* GetMoveData() const override;
	virtual FGameplayTag        GetActiveStateTag() const override;
	virtual void                BeginMovement() override;

private:

	void DashTick();

	/** 把冲刺方向（基类 AimDirection）朝 Desired 旋转，本帧至多 MaxStepDeg 度（冲刺跟随用）。 */
	void SteerAimDirectionToward(const FVector& Desired, float MaxStepDeg);

	/** 已冲刺距离（cm），达到 MaxDistance 时结束。 */
	float DistanceTraveled = 0.f;

	/**
	 * 冲刺起步瞬间就已重叠的目标。这些目标不触发 bStopOnHit 的撞停（仍会吃伤害），
	 * 避免贴脸发起冲撞时第一帧立刻撞停卡在原地。
	 */
	TSet<TWeakObjectPtr<AActor>> InitialOverlapActors;
};

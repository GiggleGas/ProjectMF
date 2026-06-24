// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_AIAttackBase.h"
#include "GA_GroundSlam.generated.h"

/**
 * 撼地（GroundSlam）——原地多段径向 AOE。
 *
 * 不写新逻辑：完全复用近战攻击管线的「波次模式」
 * （UGA_AIAttackBase + UMFAttackAbilityData::bWaveMode + Waves）。
 * 本类仅赋予独立 AbilityTag（MF.Ability.Pet.Move.GroundSlam）作为激活 / 冷却身份，与基础近战区分。
 *
 * 配置：AttackData 勾选 bWaveMode，填 Waves（每波 Interval/Radius/可选 InnerRadius/DamageScale）；
 * 其余（攻击动画 / 伤害 GE / 前摇 HitDelaySeconds / 冷却）与近战相同。落点圆预警由基类自动驱动。
 *
 * StateTree 经 STTask_ActivateAttack 以 AbilityTag=MF.Ability.Pet.Move.GroundSlam 激活。
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_GroundSlam : public UGA_AIAttackBase
{
	GENERATED_BODY()

public:

	UGA_GroundSlam();
};

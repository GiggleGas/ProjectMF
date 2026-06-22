// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "MFPetGameplayAbility.generated.h"

struct FMFOnHitEffect;
class UMFAttackDataBase;

/**
 * AI 战斗技能的归属基类（拥有者轴）。
 *
 * 涵盖所有由 AI 控制的战斗者——宠物、敌人、Boss——释放的技能
 * （近战 / 远程 / 移动技能 等）。集中实现：命中附加效果、技能冷却（GAS 原生）。
 *
 * 冷却：构造里把 CooldownGameplayEffectClass 设为共享的 UMFCooldownGameplayEffect；
 * 冷却时长读自数据资产 GetAttackDataBase()->CooldownSeconds；冷却身份用技能自身 AbilityTag。
 */
UCLASS(Abstract)
class PROJECTMF_API UMFPetGameplayAbility : public UMFGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UMFPetGameplayAbility();

	/** 本技能在该 ActorInfo 上的剩余冷却（秒）；无冷却返回 0。供 UI / 查询用。 */
	float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const;

protected:
	/**
	 * 返回本技能的攻击数据基类（子类覆写返回各自数据资产：近战 AttackData / 远程 GetRangedData /
	 * 移动 GetMoveData）。基类据此读取 CooldownSeconds 等共享字段。默认 nullptr。
	 */
	virtual UMFAttackDataBase* GetAttackDataBase() const { return nullptr; }

	/**
	 * 对 Target 逐条施加「命中附加效果」：按 Chance roll，命中则 MakeOutgoingSpec(Effect)，
	 * 用 SetByCaller(MF.Data.Duration) 写入时长后施加到目标。近战与远程攻击共用。
	 */
	void ApplyOnHitEffects(AActor* Target, const TArray<FMFOnHitEffect>& Effects);

	// --- 冷却（GAS 原生重写）---

	/** 冷却身份标签 = 技能自身 AbilityTag（唯一），供 CanActivateAbility 拦截。 */
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	/** 施加冷却：读 GetAttackDataBase()->CooldownSeconds(>0)，写 SetByCaller 时长 + 动态授予 AbilityTag。 */
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
};

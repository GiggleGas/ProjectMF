// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "STCond_CanAutoUseSkill.generated.h"

/**
 * InstanceData：
 *   AbilityTag — 要判定的技能 AbilityTag（与同状态的 STTask_ActivateAttack 配同一个 tag）。
 */
USTRUCT()
struct PROJECTMF_API FSTCond_CanAutoUseSkill_InstanceData
{
	GENERATED_BODY()

	/** 要判定能否"自动释放"的技能标签。 */
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (Categories = "MF.Ability"))
	FGameplayTag AbilityTag;
};

/**
 * STCond_CanAutoUseSkill — 判定 AI 此刻能否自动释放某技能。
 *
 * 成立条件（全部满足）：
 *   1. 已授予 AbilityTag 对应技能；
 *   2. 不在冷却（owner 不持有该技能的冷却身份标签）；
 *   3. 非召唤宠物（敌人/野宠）→ 直接允许；召唤宠物 → 仅当该 spec 标了 MF.SkillMode.Auto。
 *
 * 用作宠物 StateTree（ST_PetRoot）自动攻击节点的进入条件——这样"默认手动"只约束召唤宠物，
 * 敌人/野宠不受影响（其自动攻击节点同样可用此条件，因非召唤恒返回 true，或干脆不挂）。
 */
USTRUCT(DisplayName = "MF Can Auto Use Skill")
struct PROJECTMF_API FSTCond_CanAutoUseSkill : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

public:
	using FInstanceDataType = FSTCond_CanAutoUseSkill_InstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

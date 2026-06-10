// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFAttackTypes.h"
#include "MFAttackDataBase.generated.h"

class UGameplayEffect;
class UPaperZDAnimSequence;

/**
 * 所有攻击技能数据资产的共享基类。
 *
 * 集中放置近战与远程攻击都需要的通用字段：攻击动画、伤害 GE、伤害倍率、
 * 阵营过滤。子类只补充各自专有的形状 / 物理 / 弹幕参数。
 *
 * 继承链：
 *   UMFAttackDataBase
 *     ├── UMFAttackAbilityData        （近战 / AOE）
 *     └── UMFRangedAttackDataBase     （远程公共）
 *           ├── UMFProjectileAttackData
 *           ├── UMFFallingBoulderData
 *           └── UMFBulletCurtainData
 */
UCLASS(Abstract, BlueprintType)
class PROJECTMF_API UMFAttackDataBase : public UDataAsset
{
	GENERATED_BODY()

public:

	/** 技能激活时播放的动画。可空——为空时仅靠计时器驱动伤害。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Anim")
	TObjectPtr<UPaperZDAnimSequence> AttackAnim;

	/**
	 * 命中有效目标时施加的伤害 GameplayEffect。
	 * 该 GE 须通过 SetByCaller（标签 MF.Attack.Data.Damage）读取伤害量。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageGE;

	/** 伤害倍率，作为 SetByCaller 值写入 GE Spec（最终伤害 = Attack 属性 × 此值）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	/** 决定攻击影响敌方、友方还是全部角色。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;

	/**
	 * 命中有效目标时，在伤害之外按概率施加的附加效果（眩晕 / 减速 / 易伤…）。
	 * 近战与远程（含落石 AOE 每个目标）均支持；每个目标独立 roll 概率。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|OnHit")
	TArray<FMFOnHitEffect> OnHitEffects;
};

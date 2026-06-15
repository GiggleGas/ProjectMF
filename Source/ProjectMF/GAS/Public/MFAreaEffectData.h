// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFAttackTypes.h"   // FMFOnHitEffect, EAttackTargetFilter
#include "MFAreaEffectData.generated.h"

class UGameplayEffect;

/**
 * 持续区域效果（"场"）的配置。
 *
 * 由 UMFCombatStatics::SpawnAreaEffect 生成、UMFAreaEffectSubsystem 驱动：
 * 在半径内每 TickInterval 对通过过滤的目标 ①造成一次伤害（DamageMultiplier>0 时，复用瞬时伤害路径）
 * ②施加状态效果（Effects，复用 OnHitEffects 映射表）。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFAreaEffectData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 区域半径（cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "1.0"))
	float Radius = 300.f;

	/** 区域存活时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0.1"))
	float Duration = 5.f;

	/** 施加间隔（秒）：每隔此时间对范围内目标施加一次。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0.05"))
	float TickInterval = 0.5f;

	/** 影响敌方/友方/全部。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area")
	EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;

	// -----------------------------------------------------------------------
	// 伤害（复用瞬时伤害路径；按来源 Attack × DamageMultiplier。0 = 不造成伤害）
	// -----------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area|Damage")
	TSubclassOf<UGameplayEffect> DamageGE;

	/** 每 tick 伤害倍率（最终 = 来源 Attack × 此值）。0 = 该区域不造成伤害。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area|Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 0.f;

	// -----------------------------------------------------------------------
	// 状态效果（每 tick 施加；用 Kind+映射表，与攻击 OnHitEffects 同一套）
	// 控制类 GE 请配 Stacking=刷新/上限1，否则会逐 tick 叠层。
	// -----------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area|Effects")
	TArray<FMFOnHitEffect> Effects;

	// -----------------------------------------------------------------------
	// 表现：随场一起生成的展示 Actor（建议继承 AMFSceneActorBase，BP 里放 PaperZD/Flipbook，如火圈循环）。
	// 由子系统在生成时 Spawn、区域结束/取消时销毁。可空 = 无表现。
	// -----------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area|Visual")
	TSubclassOf<AActor> VisualActorClass;

	/** 该视觉在缩放 1.0 时对应的半径(cm)。>0 时子系统按 Radius/此值 自动等比缩放视觉以匹配区域；0=不缩放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area|Visual", meta = (ClampMin = "0.0"))
	float VisualBaseRadius = 0.f;
};

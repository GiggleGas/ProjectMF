// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFAttackTypes.h"
#include "MFAttackAbilityData.generated.h"

class UGameplayEffect;

/**
 * Data Asset：AI 攻击技能的所有可配置参数。
 *
 * 使用方式：
 *   1. 内容浏览器右键 → 杂项 → 数据资产 → 选择 MFAttackAbilityData
 *   2. 将创建的资产赋值给具体 GA 蓝图子类的 AttackData 属性
 *
 * 所有数值通过此资产驱动，运行时可直接修改，无需重新编译。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFAttackAbilityData : public UDataAsset
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// 检测形状
	// -----------------------------------------------------------------------

	/** 检测体的形状类型。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Detection")
	EAttackShapeType ShapeType = EAttackShapeType::Sphere;

	/**
	 * 检测起点相对于角色 Actor 根节点的偏移（本地空间）。
	 * 运行时按角色当前朝向旋转后叠加到 Actor 位置。
	 * 例：FVector(80, 0, 0) 表示向前偏移 80 单位。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Detection")
	FVector DetectionOffset = FVector::ZeroVector;

	/**
	 * 球形检测半径 / 扇形检测距离（单位：cm）。
	 * Box 形状时作为 BoxHalfExtent.X 使用。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Detection",
		meta = (ClampMin = "1.0"))
	float Range = 150.f;

	/**
	 * 扇形检测的半角（度）。仅 Sector 形状有效。
	 * 60 = 前方 120° 扇形；180 = 半圆。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Detection",
		meta = (ClampMin = "1.0", ClampMax = "180.0", EditCondition = "ShapeType == EAttackShapeType::Sector"))
	float HalfAngle = 60.f;

	/**
	 * 矩形检测的半尺寸（XYZ 各轴，单位：cm）。仅 Box 形状有效。
	 * Box 为 AABB（轴对齐），需要有向矩形请在子类覆写 CollectTargets。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Detection",
		meta = (EditCondition = "ShapeType == EAttackShapeType::Box"))
	FVector BoxHalfExtent = FVector(100.f, 60.f, 60.f);

	// -----------------------------------------------------------------------
	// 目标过滤
	// -----------------------------------------------------------------------

	/** 决定攻击影响敌方、友方还是全部角色。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Target")
	EAttackTargetFilter TargetFilter = EAttackTargetFilter::EnemyOnly;

	// -----------------------------------------------------------------------
	// 时序控制
	// -----------------------------------------------------------------------

	/**
	 * 技能激活到第一次触发伤害检测的延迟（秒）。
	 * 应与动画攻击帧对齐，例如攻击动作在 0.3s 落手则设为 0.3。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Timing",
		meta = (ClampMin = "0.0"))
	float HitDelaySeconds = 0.3f;

	/**
	 * 技能总时长（秒）。仅在单次打击模式（非多段、非持续）下有效，
	 * 控制技能何时调用 EndAbility。应 >= HitDelaySeconds。
	 * 多段和持续模式由各自的计数/时长决定，忽略此值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Timing",
		meta = (ClampMin = "0.0"))
	float AbilityDuration = 0.8f;

	// -----------------------------------------------------------------------
	// 多段伤害（单次释放内打击多次，如旋转攻击）
	// -----------------------------------------------------------------------

	/** 开启后此次技能会打击 HitCount 次，每次间隔 HitInterval。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|MultiHit")
	bool bMultiHit = false;

	/** 总打击次数（含第一次）。bMultiHit=true 时有效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|MultiHit",
		meta = (ClampMin = "2", EditCondition = "bMultiHit"))
	int32 HitCount = 3;

	/** 每两次打击之间的间隔（秒）。bMultiHit=true 时有效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|MultiHit",
		meta = (ClampMin = "0.05", EditCondition = "bMultiHit"))
	float HitInterval = 0.2f;

	// -----------------------------------------------------------------------
	// 持续伤害（场地毒云、火焰持续燃烧等）
	// -----------------------------------------------------------------------

	/**
	 * 开启后技能会在 SustainedDuration 内每隔 TickInterval 检测并施加一次伤害。
	 * bMultiHit 和 bSustained 互斥，同时开启时 bSustained 优先。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Sustained")
	bool bSustained = false;

	/** 持续伤害总时长（秒）。bSustained=true 时有效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Sustained",
		meta = (ClampMin = "0.1", EditCondition = "bSustained"))
	float SustainedDuration = 3.f;

	/** 每次伤害 Tick 之间的间隔（秒）。bSustained=true 时有效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Sustained",
		meta = (ClampMin = "0.05", EditCondition = "bSustained"))
	float TickInterval = 0.5f;

	// -----------------------------------------------------------------------
	// 伤害
	// -----------------------------------------------------------------------

	/**
	 * 应用到目标的伤害 GameplayEffect。
	 * 该 GE 应配置为通过 SetByCaller（标签 MF.Attack.Data.Damage）读取伤害量。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageGameplayEffect;

	/**
	 * 伤害倍率，作为 SetByCaller 值写入 GE Spec。
	 * 最终伤害由 GE 内部公式（可引用 Attack 属性）乘以此值决定。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage",
		meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;
};

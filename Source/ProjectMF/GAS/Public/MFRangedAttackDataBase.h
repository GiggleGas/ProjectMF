// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackDataBase.h"
#include "MFRangedAttackDataBase.generated.h"

class UStaticMesh;

/**
 * 远程攻击数据资产的共享基类。
 *
 * 在通用攻击字段（见 UMFAttackDataBase）之上，补充所有远程攻击共有的
 * 投射物物理参数与出手延迟。具体远程技能（投掷 / 落石 / 弹幕）只补充
 * 各自专有的字段。
 *
 * 落石（UMFFallingBoulderData）复用语义：
 *   MaxRange       = 下落高度（生成点离地高度，也是落地判定距离）
 *   Speed          = 下落速度
 *   ProjectileMesh = 落石网格
 */
UCLASS(Abstract, BlueprintType)
class PROJECTMF_API UMFRangedAttackDataBase : public UMFAttackDataBase
{
	GENERATED_BODY()

public:

	/** 技能激活到投射物 Spawn 之间的延迟（秒），应与出手动画帧对齐。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Timing", meta = (ClampMin = "0.0"))
	float AnimToSpawnDelay = 0.2f;

	/** 投射物飞行速度（cm/s）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Projectile", meta = (ClampMin = "1.0"))
	float Speed = 1000.f;

	/** 投射物最大飞行距离（cm），超过则消散。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Projectile", meta = (ClampMin = "1.0"))
	float MaxRange = 1500.f;

	/** 命中检测的扫掠球半径（cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Projectile", meta = (ClampMin = "1.0"))
	float CollisionRadius = 15.f;

	/** 通过 ISM 渲染的投射物网格。留空则为隐形投射物。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;
};

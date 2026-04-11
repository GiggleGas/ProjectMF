// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFCatchPetConfig.generated.h"

/**
 * Data Asset：抓宠技能的所有可配置参数。
 *
 * 使用方式：
 *   1. 在内容浏览器中右键 → 杂项 → 数据资产 → 选择 MFCatchPetConfig
 *   2. 将创建的资产赋值给 BP_GA_CatchPet 的 CatchConfig 属性
 *
 * 所有数值通过此资产驱动，不硬编码在代码中。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFCatchPetConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// 弹反 QTE 参数
	// -----------------------------------------------------------------------

	/** 成功收服宠物所需的弹反次数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|QTE",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxBounceCount = 3;

	/** 每次球到达玩家后，玩家必须在此时间内按下 Space，否则失败（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|QTE",
		meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float QTETimeLimit = 1.5f;

	// -----------------------------------------------------------------------
	// 球的运动参数
	// -----------------------------------------------------------------------

	/** 球在绳子上弹射的速度（单位/秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Ball",
		meta = (ClampMin = "100.0"))
	float BallSpeed = 800.f;

	// -----------------------------------------------------------------------
	// 瞄准参数
	// -----------------------------------------------------------------------

	/** 预瞄线的最大长度（单位）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Aim",
		meta = (ClampMin = "100.0"))
	float AimLineLength = 2000.f;

	/**
	 * 鼠标下宠物检测的球形追踪半径（单位）。
	 * 值越大越容易命中，但可能误选到相邻宠物。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Aim",
		meta = (ClampMin = "0.0"))
	float TargetTraceRadius = 40.f;

	// -----------------------------------------------------------------------
	// 高亮描边（CustomDepthStencil）
	// 需要在后处理材质中读取 CustomDepth Stencil 值来输出对应颜色轮廓线。
	// -----------------------------------------------------------------------

	/** 可抓取宠物的 Stencil 值（后处理材质输出白色描边）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Highlight",
		meta = (ClampMin = "0", ClampMax = "255"))
	int32 CatchableStencilValue = 1;

	/** 不可抓取宠物的 Stencil 值（后处理材质输出红色描边）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Highlight",
		meta = (ClampMin = "0", ClampMax = "255"))
	int32 UncatchableStencilValue = 2;

	// -----------------------------------------------------------------------
	// 生成类（运行时 Spawn）
	// -----------------------------------------------------------------------

	/** 绳索 Actor 的类，留空则不生成绳索视觉。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Actors")
	TSubclassOf<class ACatchRopeActor> RopeActorClass;

	/** 弹射球 Actor 的类，留空则不生成球视觉。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Actors")
	TSubclassOf<class ACatchBallActor> BallActorClass;
};

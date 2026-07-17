// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFThrowableData.generated.h"

class UPaperFlipbook;
class UMFAreaEffectData;

/**
 * UMFThrowableData — 投掷消耗品参数（抛物线投掷物）。
 *
 * 消耗品 FMFItemDef 配了 ThrowableData = **投掷模式**：玩家瞄准地面 → 以抛物线扔出一个
 * Flipbook 投射物（走投射物子系统的 Flipbook 池渲染）→ 落地时生成 ImpactArea（回血场等）。
 *
 * 弹道用「速度 + 弧高」两个直观旋钮：投掷入口据此反算抛物线初速度 + 重力（见 AMFCharacter::LaunchThrowable）。
 * 见 Docs/ThrowableConsumable_Design.md。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFThrowableData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 飞行外观（Flipbook 池渲染，面向相机；单帧=静态图，多帧=飞行动画）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable")
	TObjectPtr<UPaperFlipbook> FlightFlipbook;

	/** 水平飞行速度（cm/s）。飞行时长 = 水平距离 / Speed，越大扔得越快。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable", meta = (ClampMin = "100.0"))
	float Speed = 900.f;

	/** 最远投掷距离（cm）。玩家落点超此值会被夹到最远处。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable", meta = (ClampMin = "100.0"))
	float MaxRange = 1500.f;

	/** 抛物线顶高（cm）。控制弧度高低，与 Speed 独立。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable", meta = (ClampMin = "0.0"))
	float ArcHeight = 250.f;

	/** 投射物视觉缩放（1 = flipbook 原始大小）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable", meta = (ClampMin = "0.1"))
	float VisualScale = 1.f;

	/** 落地时在落点生成的区域效果（回血场等）。空 = 落地无效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable")
	TObjectPtr<UMFAreaEffectData> ImpactArea;
};

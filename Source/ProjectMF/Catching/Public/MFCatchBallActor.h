// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFSceneActorBase.h"
#include "MFCatchBallActor.generated.h"

/**
 * ACatchBallActor — 抓宠弹射球（2D Flipbook 表现）。
 *
 * 外观走基类 AMFSceneActorBase 的 FlipbookComponent（在 BP 子类配球的 flipbook）；
 * billboard 面向相机由基类 UMFSpriteVisualComponent 接管。
 * 位置不自插值——由 UAbilityTask_MoveBall 每帧 SetBallWorldLocation() 推送。
 */
UCLASS()
class PROJECTMF_API ACatchBallActor : public AMFSceneActorBase
{
	GENERATED_BODY()

public:
	ACatchBallActor();

	// -----------------------------------------------------------------------
	// 位置接口（供 MoveBall Task 调用）
	// -----------------------------------------------------------------------

	/** 将球移动到指定世界坐标（由 AbilityTask 每帧调用）。不做插值，调用方负责插值计算。 */
	UFUNCTION(BlueprintCallable, Category = "Catching|Ball")
	void SetBallWorldLocation(const FVector& NewLocation);

	/** 返回球当前的世界坐标。 */
	UFUNCTION(BlueprintPure, Category = "Catching|Ball")
	FVector GetBallWorldLocation() const;

protected:
	virtual void BeginPlay() override;
};

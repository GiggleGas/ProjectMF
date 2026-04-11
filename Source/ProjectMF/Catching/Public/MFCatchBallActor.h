// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MFCatchBallActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * ACatchBallActor — 抓宠弹射球
 *
 * 负责球的视觉表现。位置插值由外部 UAbilityTask_MoveBall（下一批实现）驱动，
 * 通过 SetBallWorldLocation() 每帧推送位置。
 *
 * 当前批次只实现：
 *   - 球的组件构建（碰撞根 + 静态网格）
 *   - SetBallWorldLocation / GetBallWorldLocation 接口
 *
 * 弹射逻辑（插值、速度、反弹事件）在 UAbilityTask_MoveBall 中实现。
 */
UCLASS()
class PROJECTMF_API ACatchBallActor : public AActor
{
	GENERATED_BODY()

public:
	ACatchBallActor();

	// -----------------------------------------------------------------------
	// 位置接口（供 MoveBall Task 调用）
	// -----------------------------------------------------------------------

	/**
	 * 将球移动到指定世界坐标（由 AbilityTask 每帧调用）。
	 * 不做插值，调用方负责插值计算。
	 */
	UFUNCTION(BlueprintCallable, Category = "Catching|Ball")
	void SetBallWorldLocation(const FVector& NewLocation);

	/** 返回球当前的世界坐标。 */
	UFUNCTION(BlueprintPure, Category = "Catching|Ball")
	FVector GetBallWorldLocation() const;

	// -----------------------------------------------------------------------
	// 组件
	// -----------------------------------------------------------------------

	/** 碰撞根（球形）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereCollision;

	/** 球的静态网格（在 BP 子类中指定网格资产）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BallMesh;

protected:
	virtual void BeginPlay() override;
};

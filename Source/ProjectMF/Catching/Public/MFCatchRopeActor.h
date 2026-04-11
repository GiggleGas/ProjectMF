// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MFCatchRopeActor.generated.h"

class USplineComponent;

/**
 * ACatchRopeActor — 抓宠绳索视觉表现
 *
 * 通过 USplineComponent 在玩家和目标宠物之间建立连接线。
 * 每帧 Tick 更新两端点位置以跟随角色/宠物移动。
 *
 * 原型阶段：用 DrawDebugLine 显示绳索，后期可替换为 SplineMeshComponent + 网格。
 *
 * 使用方式（由 GA_CatchPet 调用）：
 *   RopeActor = World->SpawnActor<ACatchRopeActor>(...);
 *   RopeActor->SetEndpoints(PlayerActor, PetActor);
 */
UCLASS()
class PROJECTMF_API ACatchRopeActor : public AActor
{
	GENERATED_BODY()

public:
	ACatchRopeActor();

	// -----------------------------------------------------------------------
	// 初始化接口
	// -----------------------------------------------------------------------

	/**
	 * 设置绳索两端的 Actor（玩家角色 + 目标宠物）。
	 * 必须在 Spawn 之后立即调用，否则 Tick 无法更新端点。
	 */
	UFUNCTION(BlueprintCallable, Category = "Catching|Rope")
	void SetEndpoints(AActor* InStart, AActor* InEnd);

	// -----------------------------------------------------------------------
	// USplineComponent（可在 BP 子类中挂载 SplineMeshComponent）
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> RopeSpline;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** 绳索起点 Actor（通常是玩家角色）。 */
	TWeakObjectPtr<AActor> StartActor;

	/** 绳索终点 Actor（目标宠物）。 */
	TWeakObjectPtr<AActor> EndActor;

	/** 将 Spline 的两个控制点更新为当前 StartActor / EndActor 的位置。 */
	void UpdateSplinePoints();
};

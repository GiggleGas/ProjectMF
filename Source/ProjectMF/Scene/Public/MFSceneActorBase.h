// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MFSceneActorBase.generated.h"

class UPaperFlipbookComponent;
class UPaperZDAnimationComponent;

/**
 * 场景 Actor 渲染基类：一个能播放 PaperZD / PaperFlipbook 2D 帧动画的轻量 Actor。
 *
 * 只负责「怎么渲染」——提供 Flipbook + PaperZD 动画组件；不含移动 / GAS / 碰撞 / 自销毁，
 * 也不主动朝向相机（朝向交由资产作者或子类决定）。
 *
 * 「谁来生成 / 销毁（寿命）」由各使用方负责，本基类不自管：
 *   - 区域表现（火圈等）：由 UMFAreaEffectSubsystem 生成、到时/取消时销毁（有持续时间）。
 *   - 树木 / 矿石等可采集物：由资源/世界生成系统生成、采集或耗尽时销毁（持久到被采）。
 * 后者将派生子类追加碰撞与交互（TODO）。
 */
UCLASS(Abstract, Blueprintable)
class PROJECTMF_API AMFSceneActorBase : public AActor
{
	GENERATED_BODY()

public:
	AMFSceneActorBase();

	UPaperFlipbookComponent*    GetFlipbookComponent()  const { return FlipbookComponent; }
	UPaperZDAnimationComponent* GetAnimationComponent() const { return AnimationComponent; }

protected:
	/** 2D 渲染目标（由 PaperZD 驱动；简单单循环表现也可直接设其 Flipbook 不用 PaperZD）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	/** PaperZD 动画组件（在 BP 设 AnimBP 类即可用状态机；纯单循环可不设）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperZDAnimationComponent> AnimationComponent;
};

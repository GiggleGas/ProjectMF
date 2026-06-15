// Copyright ProjectMF. All Rights Reserved.

#include "MFSceneActorBase.h"
#include "PaperFlipbookComponent.h"
#include "PaperZDAnimationComponent.h"

AMFSceneActorBase::AMFSceneActorBase()
{
	// 纯展示：不需要逐帧 Actor tick（Flipbook 组件自身会推进动画）。
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// --- Flipbook（PaperZD 的渲染目标）---
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetupAttachment(SceneRoot);
	FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // 表现层默认无碰撞；需要碰撞的子类自行追加

	// --- PaperZD 动画 ---
	// 在派生 BP 里设置 AnimBP 类即可使用状态机；纯单循环表现可不设，直接给 Flipbook 一个循环动画。
	AnimationComponent = CreateDefaultSubobject<UPaperZDAnimationComponent>(TEXT("AnimationComponent"));
	AnimationComponent->InitRenderComponent(FlipbookComponent);
}

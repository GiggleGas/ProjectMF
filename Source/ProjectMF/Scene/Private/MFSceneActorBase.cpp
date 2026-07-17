// Copyright ProjectMF. All Rights Reserved.

#include "MFSceneActorBase.h"
#include "MFSpriteVisualComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperZDAnimationComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

AMFSceneActorBase::AMFSceneActorBase()
{
	// 纯展示：不需要逐帧 Actor tick（Flipbook 组件自身会推进动画，billboard 由组件自身 tick 驱动）。
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

	// --- 2D 表现能力组件（billboard 面向相机；组件自身 tick 驱动） ---
	SpriteVisual = CreateDefaultSubobject<UMFSpriteVisualComponent>(TEXT("SpriteVisual"));
}

void AMFSceneActorBase::BeginPlay()
{
	Super::BeginPlay();

	if (SpriteVisual)
	{
		// 场景物默认无胶囊 → 碰撞拟合跳过；billboard/闪光用同一 Flipbook。
		SpriteVisual->InitVisual(GetVisualFlipbook(), /*Capsule=*/nullptr);
		SpriteVisual->bDriveBillboardOnTick = true;

		// 场景物相机源：玩家相机管理器朝向（与 AI 一致，全场景统一倾斜）。
		TWeakObjectPtr<AMFSceneActorBase> WeakThis(this);
		SpriteVisual->SetCameraForwardProvider([WeakThis](FVector& OutForward) -> bool
		{
			if (!WeakThis.IsValid()) return false;
			if (const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(WeakThis.Get(), 0))
			{
				OutForward = PCM->GetCameraRotation().Vector();
				return true;
			}
			return false;
		});
	}
}


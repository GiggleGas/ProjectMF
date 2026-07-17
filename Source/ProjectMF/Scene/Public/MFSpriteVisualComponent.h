// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFSpriteVisualComponent.generated.h"

class UPaperFlipbookComponent;
class UCapsuleComponent;

/**
 * UMFSpriteVisualComponent — 2D sprite 表现能力组件（统一 Flipbook）。
 *
 * 把原先散在 AMFCharacterBase 里、又被掉落物重造的三块**纯表现**能力抽成一份可复用组件，
 * 供角色（CharacterBase）与场景物（SceneActorBase）共享：
 *   ① billboard          —— 让目标 Flipbook 恒面向相机
 *   ② 受击/治疗闪光       —— 短暂染色后自动复位白色
 *   ③ 碰撞自适应          —— 从 Flipbook 首帧尺寸拟合碰撞球
 *
 * 相机朝向来源用委托注入，解耦"谁提供相机"：
 *   玩家 → CameraComponent 前向；AI/场景物 → PlayerCameraManager。
 *
 * 见 Docs/SceneObject_UnifiedSprite_Architecture.md（S1）。
 *
 * S1 阶段本组件仅作为「附加能力」挂在角色上、由 exec 命令（MFSVBillboard/MFSVFlash/MFSVCollision）
 * 单独验证，不自动接管每帧 billboard（bDriveBillboardOnTick 默认 false），避免与 CharacterBase
 * 现有逻辑双重 SetRotation 打架。S2 才切换为组件接管并删除旧实现。
 */
UCLASS(ClassGroup=(MF), meta=(BlueprintSpawnableComponent))
class PROJECTMF_API UMFSpriteVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFSpriteVisualComponent();

	/**
	 * 绑定要驱动的 Flipbook（全项目场景物体统一 Flipbook，见 Docs/SceneObject_UnifiedSprite_Architecture.md）。
	 * @param InFlipbook  billboard 旋转 + 闪光染色 + 碰撞拟合的目标 Flipbook。
	 * @param InCapsule   碰撞自适应写入的胶囊（可空；场景物无胶囊时传 nullptr，跳过碰撞拟合）。
	 */
	void InitVisual(UPaperFlipbookComponent* InFlipbook, UCapsuleComponent* InCapsule);

	/**
	 * 注入相机前向来源。委托返回 false 表示当前拿不到相机 → billboard 当帧跳过。
	 * 例：玩家传 CameraComponent->GetForwardVector()；AI 传 PlayerCameraManager 朝向。
	 */
	void SetCameraForwardProvider(TFunction<bool(FVector&)> InProvider) { CameraForwardProvider = MoveTemp(InProvider); }

	/** 让 billboard 目标面向相机（与原 AMFCharacterBase::UpdateBillboard 同算法，泛化到任意场景组件）。 */
	void TickBillboard();

	/** 闪一下指定颜色并在 Duration 秒后复位白色（受击闪红 / 治疗闪绿共用）。 */
	void FlashColor(const FLinearColor& Color, float Duration);

	/**
	 * 从 Flipbook 首帧尺寸自适应碰撞球（与原 UpdateCollisionFromFlipbook 同算法）。
	 * NewRadius = (SpriteWidth/2) * RadiusScale；同时把 Flipbook 下移 Radius 使视觉贴合物理落脚点。
	 * 无绑定胶囊则跳过（仅 log）。
	 */
	void FitCollisionToFlipbook(float RadiusScale);

	/**
	 * 是否由本组件 TickComponent 自动每帧驱动 billboard。
	 * S1 默认 false（旧逻辑仍在跑，避免打架）；S2 接管后由宿主置 true。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MF|SpriteVisual")
	bool bDriveBillboardOnTick = false;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** 闪光复位：染回白色。由 FlashColor 的定时器回调。 */
	void ResetColorToWhite();

	/** billboard 旋转 + 闪光染色 + 碰撞拟合的目标 Flipbook。 */
	UPROPERTY()
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	/** 碰撞拟合写入目标（可空）。 */
	TWeakObjectPtr<UCapsuleComponent> TargetCapsule;

	/** 相机前向来源（宿主注入）。 */
	TFunction<bool(FVector&)> CameraForwardProvider;

	FTimerHandle FlashTimerHandle;
};

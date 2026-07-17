// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MFProjectileRenderer.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;

/**
 * Scene-unique Actor that owns a pool of UPaperFlipbookComponent — one component per
 * concurrently-active projectile. Spawned and owned by UMFProjectileSubsystem; never
 * manually placed in a level.
 *
 * 全项目投射物统一 2D Flipbook 表现（基底，见 Docs/SceneObject_UnifiedSprite_Architecture.md）。
 * Paper2D 无 instanced 组件，故用「组件池」而非 ISM 批渲：slot 复用、只增不删、隐藏空闲。
 *
 * Slot 管理：
 *   AcquireSlot  — 取一个空闲组件，设 Flipbook + 世界位置 + billboard，显示；返回稳定索引。
 *   UpdateSlot   — 每 Subsystem Tick 重定位（朝向重算 billboard 面向相机）。
 *   ReleaseSlot  — 隐藏组件并把索引还回空闲池（组件不销毁，索引恒定）。
 *
 * billboard：RefreshCamera 每帧由子系统刷新一次相机前向，AcquireSlot/UpdateSlot 复用它算朝向。
 */
UCLASS(NotBlueprintable, NotPlaceable)
class PROJECTMF_API AMFProjectileRenderer : public AActor
{
	GENERATED_BODY()

public:
	AMFProjectileRenderer();

	/** 每帧由子系统调用一次：刷新相机前向，供本帧所有 slot 的 billboard 复用。 */
	void RefreshCamera();

	/**
	 * 取一个 slot 显示 Flipbook（复用空闲组件或新建）。
	 * 设外观 + 世界位置 + billboard 面向相机并显示。返回 slot 索引，失败 -1。
	 */
	int32 AcquireSlot(UPaperFlipbook* Flipbook, const FVector& WorldLocation, float Scale = 1.f);

	/** 更新 slot 的世界位置；朝向重算 billboard。每 Subsystem Tick 调用。 */
	void  UpdateSlot(int32 SlotIndex, const FVector& WorldLocation);

	/** 隐藏 slot 的组件并把索引还回空闲池（不销毁组件，其余索引保持有效）。 */
	void  ReleaseSlot(int32 SlotIndex);

private:
	/** Flipbook 组件池（挂本 Actor 根）。UPROPERTY 让 GC 跟踪。 */
	UPROPERTY()
	TArray<TObjectPtr<UPaperFlipbookComponent>> Pool;

	/** 空闲 slot 索引池，供复用。 */
	TArray<int32> FreeSlots;

	/** 本帧相机前向（RefreshCamera 刷新），供 billboard 计算。 */
	FVector CachedCamForward = FVector::ForwardVector;

	/** 取一个可用组件索引（复用空闲，否则新建并注册）。 */
	int32 AllocateComponent();

	/** billboard 旋转：sprite 法线朝相机（与 UMFSpriteVisualComponent 同算法）。 */
	FRotator GetBillboardRotation() const;
};

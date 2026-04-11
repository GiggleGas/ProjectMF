// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AT_WaitPetTarget.generated.h"

class UMFCatchPetConfig;

// ============================================================
// Delegates
// ============================================================

/** 命中可抓取宠物时触发（每帧更新）。bCanCatch 决定描边颜色。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FMFOnPetTargetFound,
	AActor*, Target,
	bool, bCanCatch);

/** 鼠标离开目标（切换到无效区域）时触发。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMFOnPetTargetLost);

/**
 * 玩家左键点击且当前有有效目标时触发。
 * bCanCatch=true 时才会触发此委托（不可抓不允许确认）。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMFOnPetTargetConfirmed,
	AActor*, ConfirmedTarget);

/** 玩家右键点击或按 ESC 时触发。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMFOnPetTargetCancelled);

// ============================================================
// Task
// ============================================================

/**
 * AbilityTask: 瞄准阶段宠物选取器
 *
 * 生命周期（由 UGA_CatchPet 管理）：
 *   Phase 1 开始 → ReadyForActivation() → TickTask 每帧运行
 *     → OnTargetFound / OnTargetLost（实时高亮反馈）
 *     → 左键点击有效目标 → OnTargetConfirmed → GA 进入 Phase 2
 *     → 右键 / ESC   → OnCancelled → GA 结束
 *   Phase 2 或 EndAbility → EndTask() → OnDestroy() 清理高亮
 *
 * 高亮原理：
 *   对命中的宠物 Actor 所有 UPrimitiveComponent 开启 CustomDepth，
 *   并写入 CatchableStencilValue（白色）或 UncatchableStencilValue（红色）。
 *   后处理材质读取 Stencil 值渲染对应颜色描边。
 */
UCLASS()
class PROJECTMF_API UAbilityTask_WaitPetTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// 委托（Blueprint 可绑定）
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable)
	FMFOnPetTargetFound     OnTargetFound;

	UPROPERTY(BlueprintAssignable)
	FMFOnPetTargetLost      OnTargetLost;

	UPROPERTY(BlueprintAssignable)
	FMFOnPetTargetConfirmed OnTargetConfirmed;

	UPROPERTY(BlueprintAssignable)
	FMFOnPetTargetCancelled OnCancelled;

	// -----------------------------------------------------------------------
	// 工厂函数
	// -----------------------------------------------------------------------

	/**
	 * 创建并返回 Task 实例（在 ReadyForActivation 之前绑定委托）。
	 *
	 * @param OwningAbility    持有此 Task 的 Ability。
	 * @param TaskInstanceName 同一 Ability 内唯一名称（防重复实例）。
	 * @param InConfig         抓宠配置 DA，不可为 null。
	 */
	UFUNCTION(BlueprintCallable,
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",
		        BlueprintInternalUseOnly = "true"),
		Category = "MF|AbilityTask")
	static UAbilityTask_WaitPetTarget* CreateWaitPetTarget(
		UGameplayAbility*          OwningAbility,
		FName                      TaskInstanceName,
		const UMFCatchPetConfig*   InConfig);

	// -----------------------------------------------------------------------
	// UAbilityTask / UGameplayTask interface
	// -----------------------------------------------------------------------

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// -----------------------------------------------------------------------
	// 内部状态
	// -----------------------------------------------------------------------

	/** 当前命中的目标（弱引用，防止持有已销毁 Actor）。 */
	TWeakObjectPtr<AActor> CurrentTarget;

	/** 上一帧命中的目标（用于切换时清除旧高亮）。 */
	TWeakObjectPtr<AActor> PreviousTarget;

	/** 当前目标是否可抓。 */
	bool bCurrentTargetCanBeCaught = false;

	/** 配置 DA，来自工厂函数。 */
	TObjectPtr<const UMFCatchPetConfig> Config;

	/** 左键上一帧状态（用于边沿检测）。 */
	bool bWasLeftMouseDown = false;

	/** 右键上一帧状态（用于边沿检测）。 */
	bool bWasRightMouseDown = false;

	// -----------------------------------------------------------------------
	// 内部逻辑
	// -----------------------------------------------------------------------

	/** 从 Ability 的 ActorInfo 取得 PlayerController，失败返回 nullptr。 */
	APlayerController* GetOwnerController() const;

	/** 从 Ability 的 ActorInfo 取得 Avatar Actor（玩家角色）。 */
	AActor* GetOwnerAvatar() const;

	/**
	 * 射线检测 1：屏幕→世界几何（ECC_Visibility），获取鼠标落点。
	 * 命中时 OutLandingPoint = 碰撞点；未命中时延伸到 Config->AimLineLength 处。
	 * @return true = 实际碰撞到几何体，false = 延伸到最大射程。
	 */
	bool DoWorldTrace(FVector& OutLandingPoint) const;

	/**
	 * 射线检测 2：鼠标位置附近的 Pawn 检测（ECC_Pawn），返回 IMFCatchable Actor。
	 * @param bOutCanCatch  输出：目标是否可抓（仅在返回值非空时有效）。
	 * @return 命中的宠物 Actor；未命中或不实现 IMFCatchable 返回 nullptr。
	 */
	AActor* DoPetTrace(bool& bOutCanCatch) const;

	/**
	 * 将高亮描边应用到 Actor 的所有 PrimitiveComponent。
	 * @param Actor        要高亮的 Actor。
	 * @param StencilValue CustomDepth Stencil 值（1=可抓，2=不可抓）。
	 */
	static void ApplyHighlight(AActor* Actor, int32 StencilValue);

	/** 移除 Actor 所有 PrimitiveComponent 的自定义深度描边。 */
	static void RemoveHighlight(AActor* Actor);

	/**
	 * 绘制瞄准调试信息（在 ENABLE_DRAW_DEBUG 构建中有效）：
	 *   - 从脚部到落点的蓝色预瞄线（始终显示）
	 *   - 落点处球体（受 MF.Debug.CatchAim CVar 控制）：
	 *       绿色 = 光标下无宠物，黄色 = 光标下有宠物
	 * @param FeetPos      角色脚底世界坐标。
	 * @param LandingPoint DoWorldTrace 返回的落点坐标。
	 * @param bHasPet      DoPetTrace 是否找到有效宠物。
	 */
	void DrawAimDebug(const FVector& FeetPos, const FVector& LandingPoint, bool bHasPet) const;
};

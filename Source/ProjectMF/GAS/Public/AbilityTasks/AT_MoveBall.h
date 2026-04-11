// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AT_MoveBall.generated.h"

class ACatchBallActor;
class UMFCatchPetConfig;

// ============================================================
// Delegates
// ============================================================

/** 球到达玩家位置时触发，QTE 窗口开启。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMFOnBallReachedPlayer);

/** 玩家在 QTE 窗口内成功按下 Space 时触发。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMFOnQTESuccess);

/** QTE 计时超时，玩家未能及时反弹时触发 — 抓取失败。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMFOnQTEFailed);

/** 达到目标弹反次数后触发 — 抓取成功。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMFOnAllBouncesComplete);

// ============================================================
// Task
// ============================================================

/**
 * AbilityTask: 弹射球运动 + QTE 弹反
 *
 * 生命周期（由 UGA_CatchPet 管理）：
 *   Phase 2 结束后创建 → ReadyForActivation()
 *   → TickTask 驱动球在玩家脚部 ↔ 宠物之间来回移动
 *   → 球到达宠物：自动反弹回玩家（无需输入）
 *   → 球到达玩家：QTE 窗口开启（OnBallReachedPlayer），等待 Space 键
 *       ├─ Space in time: BounceCount++
 *       │     ├─ BounceCount < MaxBounce → 球再次飞向宠物（OnQTESuccess）
 *       │     └─ BounceCount >= MaxBounce → OnAllBouncesComplete（抓取成功）
 *       └─ 超时 → OnQTEFailed（抓取失败）
 *
 * 球路径：
 *   线性插值 Lerp(玩家脚部, 宠物中心, T)，T ∈ [0, 1]。
 *   绳索视觉由 ACatchRopeActor 独立维护，球只做位置跟随。
 */
UCLASS()
class PROJECTMF_API UAbilityTask_MoveBall : public UAbilityTask
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// 委托（Blueprint 可绑定）
	// -----------------------------------------------------------------------

	/** 球抵达玩家，QTE 窗口开启。 */
	UPROPERTY(BlueprintAssignable)
	FMFOnBallReachedPlayer OnBallReachedPlayer;

	/** 玩家在窗口内成功按下 Space（球反弹回宠物）。 */
	UPROPERTY(BlueprintAssignable)
	FMFOnQTESuccess OnQTESuccess;

	/** QTE 超时，抓取失败。 */
	UPROPERTY(BlueprintAssignable)
	FMFOnQTEFailed OnQTEFailed;

	/** 所有弹反次数达成，抓取成功。 */
	UPROPERTY(BlueprintAssignable)
	FMFOnAllBouncesComplete OnAllBouncesComplete;

	// -----------------------------------------------------------------------
	// 工厂函数
	// -----------------------------------------------------------------------

	/**
	 * 创建弹射球移动 Task。
	 *
	 * @param OwningAbility    持有此 Task 的 Ability。
	 * @param TaskInstanceName 唯一实例名（防重复）。
	 * @param InBall           要驱动的 ACatchBallActor。
	 * @param InOwner          发球方（玩家角色），用于计算脚部位置。
	 * @param InTarget         目标宠物，用于计算终点位置。
	 * @param InConfig         抓宠配置（BallSpeed、MaxBounceCount、QTETimeLimit）。
	 */
	UFUNCTION(BlueprintCallable,
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",
		        BlueprintInternalUseOnly = "true"),
		Category = "MF|AbilityTask")
	static UAbilityTask_MoveBall* CreateMoveBall(
		UGameplayAbility*        OwningAbility,
		FName                    TaskInstanceName,
		ACatchBallActor*         InBall,
		AActor*                  InOwner,
		AActor*                  InTarget,
		const UMFCatchPetConfig* InConfig);

	// -----------------------------------------------------------------------
	// UAbilityTask / UGameplayTask interface
	// -----------------------------------------------------------------------

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// -----------------------------------------------------------------------
	// 内部状态机
	// -----------------------------------------------------------------------

	enum class EBallPhase : uint8
	{
		MovingToPet,       // 球从玩家飞向宠物
		MovingToPlayer,    // 球从宠物反弹回玩家
		WaitingQTE,        // 球在玩家处，等待 Space 输入
	};

	// -----------------------------------------------------------------------
	// 运行时数据
	// -----------------------------------------------------------------------

	TWeakObjectPtr<ACatchBallActor>    BallActor;
	TWeakObjectPtr<AActor>             OwnerActor;   // 玩家角色（球起点）
	TWeakObjectPtr<AActor>             TargetActor;  // 目标宠物（球终点）
	TObjectPtr<const UMFCatchPetConfig> Config;

	EBallPhase BallPhase  = EBallPhase::MovingToPet;

	/** 线性插值参数：0 = 玩家脚部，1 = 宠物中心。 */
	float CurrentT        = 0.f;

	/** 已成功弹反次数（玩家按 Space 的次数）。 */
	int32 BounceCount     = 0;

	/** QTE 等待计时（秒），超过 QTETimeLimit 则失败。 */
	float QTEElapsed      = 0.f;

	/** Space 键上一帧状态（边沿检测）。 */
	bool  bWasSpaceDown   = false;

	// -----------------------------------------------------------------------
	// 位置辅助
	// -----------------------------------------------------------------------

	/** 玩家角色脚底世界坐标（考虑胶囊体半高）。 */
	FVector GetOwnerFeetPos() const;

	/** 宠物世界坐标（中心）。 */
	FVector GetTargetPos() const;

	/** 根据 T 值插值出球的世界坐标。 */
	FVector ComputeBallPos(float T) const;
};

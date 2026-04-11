// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFGameplayAbilityBase.h"
#include "GA_CatchPet.generated.h"

class UMFCatchPetConfig;
class UAbilityTask_WaitPetTarget;
class UAbilityTask_MoveBall;
class ACatchRopeActor;
class ACatchBallActor;

/**
 * GA_CatchPet — 抓宠主技能
 *
 * 阶段流程：
 *   Phase 1 (Aiming)     — 瞄准，预瞄线 + 宠物高亮
 *   Phase 2 (RopeActive) — 绳索 + 球 Spawn，立即进入 Phase 3
 *   Phase 3 (QTE)        — AT_MoveBall 驱动球运动，弹反 QTE 计数
 *
 * 配置：
 *   所有数值参数从 CatchConfig（UMFCatchPetConfig DataAsset）读取。
 *   在 Blueprint CDO 中将资产赋值到 CatchConfig 属性。
 *
 * 输入：
 *   - 抓宠键（Completed/Released） → TryActivateAbilitiesByTag(MF.Ability.CatchPet)
 *   - 左键点击（在 AT_WaitPetTarget 内轮询）→ OnTargetConfirmed
 *   - 右键点击 / ESC → OnAimCancelled
 */
UCLASS(Blueprintable)
class PROJECTMF_API UGA_CatchPet : public UMFGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_CatchPet();

	// -----------------------------------------------------------------------
	// 配置
	// -----------------------------------------------------------------------

	/**
	 * 抓宠配置 DataAsset。
	 * 在 BP_GA_CatchPet 的 Class Defaults 中赋值后才能正常运行。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catching|Config")
	TObjectPtr<UMFCatchPetConfig> CatchConfig;

	// -----------------------------------------------------------------------
	// UGameplayAbility interface
	// -----------------------------------------------------------------------

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*            TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle     Handle,
		const FGameplayAbilityActorInfo*     ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool                                 bReplicateEndAbility,
		bool                                 bWasCancelled) override;

private:
	// -----------------------------------------------------------------------
	// 阶段管理
	// -----------------------------------------------------------------------

	/** 启动 Phase 1：创建 WaitPetTarget Task，开启瞄准。 */
	void EnterPhase1_Aiming();

	/**
	 * 启动 Phase 2：绳索 + 球 Spawn，过渡到 RopeActive 状态，随后触发 Phase 3。
	 * @param InTarget  Phase 1 确认的目标宠物。
	 */
	void EnterPhase2_Rope(AActor* InTarget);

	/**
	 * 启动 Phase 3：创建 AT_MoveBall Task，驱动球在玩家 ↔ 宠物间弹射并处理 QTE。
	 * 由 EnterPhase2_Rope 在 Spawn 完成后调用。
	 */
	void EnterPhase3_MoveBall();

	/**
	 * 结束整个抓宠流程（成功或失败）。
	 * @param bSuccess  true = 收服成功，false = 失败/取消。
	 */
	void EndCatch(bool bSuccess);

	// -----------------------------------------------------------------------
	// Task 回调（UFUNCTION 必须，Dynamic Delegate 要求）
	// -----------------------------------------------------------------------

	// Phase 1 — 瞄准 Task 回调
	UFUNCTION()
	void OnTargetFound(AActor* Target, bool bCanCatch);

	UFUNCTION()
	void OnTargetLost();

	UFUNCTION()
	void OnTargetConfirmed(AActor* Target);

	UFUNCTION()
	void OnAimCancelled();

	// Phase 3 — MoveBall Task 回调
	UFUNCTION()
	void OnBallReachedPlayer();

	UFUNCTION()
	void OnQTESuccess();

	UFUNCTION()
	void OnQTEFailed();

	UFUNCTION()
	void OnAllBouncesComplete();

	// -----------------------------------------------------------------------
	// 运行时状态（弱引用避免持有悬挂指针）
	// -----------------------------------------------------------------------

	/** Phase 1 瞄准 Task（OnTargetConfirmed 时 EndTask）。 */
	TWeakObjectPtr<UAbilityTask_WaitPetTarget> Task_WaitTarget;

	/** Phase 3 弹射球移动 Task（EndCatch 时 EndTask）。 */
	TWeakObjectPtr<UAbilityTask_MoveBall> Task_MoveBall;

	/** Phase 2 确认的目标宠物。 */
	TWeakObjectPtr<AActor> TargetPet;

	/** Phase 2 Spawn 的绳索 Actor。 */
	TWeakObjectPtr<ACatchRopeActor> RopeActor;

	/** Phase 2 Spawn 的弹射球 Actor。 */
	TWeakObjectPtr<ACatchBallActor> BallActor;

	// -----------------------------------------------------------------------
	// 辅助
	// -----------------------------------------------------------------------

	/** 销毁 Phase 2 生成的 Rope 和 Ball Actor。 */
	void DestroyPhase2Actors();
};

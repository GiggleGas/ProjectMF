// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFThreatComponent.generated.h"

class UMFRadarSensingComponent;

// ============================================================
// 内部威胁记录（非 USTRUCT，无需反射）
// ============================================================

/**
 * FMFThreatRecord — 威胁列表中每个已知威胁的运行时记录。
 *
 * 生命周期：
 *   加入  → OnTargetDetected：首次进入感知范围
 *   驻留  → bInSensingRange=true：目标在感知范围内，无 GraceTimer
 *   宽限  → OnTargetLost：目标离开感知范围，GraceTimer 启动
 *   移除  → GraceTimer 到期 / Actor 被销毁 / CleanupThreatList
 */
struct FMFThreatRecord
{
	TWeakObjectPtr<AActor> Actor;

	/**
	 * 宽限计时器：目标离开感知范围后启动，到期则从威胁列表移除。
	 * 目标返回感知范围时清除此 Timer（复位为"无宽限"状态）。
	 */
	FTimerHandle GraceTimer;

	/** 当前是否在感知范围内（由 OnTargetDetected / OnTargetLost 维护）。 */
	bool bInSensingRange = false;
};

// ============================================================
// 状态枚举
// ============================================================

UENUM(BlueprintType)
enum class EMFThreatState : uint8
{
	/** 威胁列表为空 / 无可选目标。 */
	Idle   UMETA(DisplayName = "Idle"),

	/**
	 * 已锁定目标。
	 * - 目标在感知范围内：无限持有锁定，不计时。
	 * - 目标离开感知范围：GraceTimer 启动；到期前仍保持锁定（AI 可追击）；
	 *   到期后目标从威胁列表移除，自动重新评估。
	 */
	Locked UMETA(DisplayName = "Locked"),
};

// ============================================================
// 配置结构体
// ============================================================

/**
 * FMFThreatConfig — 索敌系统运行时配置，存入 UMFSpawnAIConfig DataAsset。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFThreatConfig
{
	GENERATED_BODY()

	/**
	 * 交战半径（cm）。供攻击系统判断是否在攻击距离内使用。
	 * 不影响威胁列表的管理范围（由 SensingRadius 控制）。
	 * 必须 <= UMFRadarSensingComponent::SensingRadius。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EngagementRadius = 600.f;

	/**
	 * 宽限时间（秒）。
	 * 目标离开感知范围后，在威胁列表中保留此时间。
	 * 宽限期内目标返回感知范围：取消倒计时，继续锁定。
	 * 宽限期满仍未返回：从威胁列表移除，触发重新评估。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat",
		meta = (ClampMin = "0.1", UIMin = "0.1"))
	float LockDuration = 5.f;

	/**
	 * 周期性评估间隔（秒）。
	 * Idle 时主动从威胁列表选目标；Locked 时做有效性兜底检查。
	 * 建议设为 RadarConfig.ScanInterval 的 2～5 倍。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat",
		meta = (ClampMin = "0.05", UIMin = "0.05"))
	float EvaluationInterval = 0.5f;
};

// ============================================================
// 事件委托
// ============================================================

/** 当前目标变化时广播。NewTarget 为 nullptr 表示进入 Idle。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMFTargetChangedEvent, AActor*, NewTarget);

// ============================================================
// 组件
// ============================================================

/**
 * UMFThreatComponent — 基于威胁列表 + 简单状态机的索敌组件。
 *
 * 核心设计：
 *   威胁列表（ThreatRecords）= AI 的"记忆"，存储所有已知敌对目标。
 *   目标首次进入感知范围 → 加入威胁列表。
 *   目标离开感知范围     → 启动宽限计时器（仍留在列表中，AI 可追击）。
 *   宽限期满仍未返回     → 从列表移除，触发重新评估。
 *
 * 状态机：
 *   Idle   → 威胁列表有可选目标 → Locked
 *   Locked → 当前目标的宽限期满 → Idle（或换目标）
 *   Locked → 当前目标在感知范围内 → 无限保持（不计时）
 *
 * 对外接口（StateTree / 攻击系统）：
 *   GetCurrentTarget()            — 当前锁定目标
 *   GetCurrentTargetGraceTime()   — 当前目标的宽限剩余（0 = 在感知范围内）
 *   GetThreatListActors()         — 完整威胁列表（包括宽限期内目标）
 *   OnTargetChanged               — 目标变化事件
 *   MF.AI.Perception.HasTarget    — ASC 标签，有目标时自动授予/撤销
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFThreatComponent();

	// -----------------------------------------------------------------------
	// 配置
	// -----------------------------------------------------------------------

	/** 写入运行时配置。由 AMFSpawnAIManager::SpawnSinglePet 在 Spawn 后调用。 */
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void ApplyConfig(const FMFThreatConfig& InConfig);

	// -----------------------------------------------------------------------
	// 查询接口
	// -----------------------------------------------------------------------

	/** 当前锁定目标；Idle 时返回 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "Threat")
	AActor* GetCurrentTarget() const;

	/** 是否有有效锁定目标。 */
	UFUNCTION(BlueprintPure, Category = "Threat")
	bool HasTarget() const;

	/** 当前状态（Idle / Locked）。 */
	UFUNCTION(BlueprintPure, Category = "Threat")
	EMFThreatState GetThreatState() const { return State; }

	/**
	 * 当前目标的宽限剩余时间（秒）。
	 * 目标在感知范围内时返回 0（无宽限，无限持有）。
	 * Idle 状态或 Timer 无效时返回 0。
	 */
	UFUNCTION(BlueprintPure, Category = "Threat")
	float GetCurrentTargetGraceTime() const;

	/** 返回威胁列表中所有有效 Actor（含宽限期内目标）。 */
	UFUNCTION(BlueprintPure, Category = "Threat")
	TArray<AActor*> GetThreatListActors() const;

	// -----------------------------------------------------------------------
	// 事件
	// -----------------------------------------------------------------------

	/** 当前目标变化时广播（含切换为 nullptr）。 */
	UPROPERTY(BlueprintAssignable, Category = "Threat")
	FMFTargetChangedEvent OnTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// -----------------------------------------------------------------------
	// 状态机
	// -----------------------------------------------------------------------

	EMFThreatState State = EMFThreatState::Idle;

	/** 唯一合法的状态转换入口。 */
	void TransitionTo(EMFThreatState NewState, AActor* InTarget = nullptr);
	void EnterIdle();
	void EnterLocked(AActor* Target);

	// -----------------------------------------------------------------------
	// 目标评估
	// -----------------------------------------------------------------------

	/**
	 * 清理无效记录后，从威胁列表选出最优目标并调用 TransitionTo。
	 * 从威胁列表评分（含宽限期内目标），不局限于感知范围内。
	 */
	void EvaluateTargets();

	/**
	 * 对候选目标打分：越近越高（无范围限制，威胁列表内所有目标均参与）。
	 * 扩展点：叠加 Boss 权重、仇恨值等。
	 */
	float ScoreTarget(AActor* Target, const FVector& OwnerLoc) const;

	/** 设置当前目标，管理 ASC 标签并广播事件。新旧相同时为空操作。 */
	void SetCurrentTarget(AActor* NewTarget);

	// -----------------------------------------------------------------------
	// 威胁列表管理
	// -----------------------------------------------------------------------

	/** 查找指定 Actor 的记录；不存在时返回 nullptr。 */
	FMFThreatRecord* FindRecord(const AActor* Actor);
	const FMFThreatRecord* FindRecord(const AActor* Actor) const;

	/** 移除所有 Actor 已失效的记录（同时清理其 GraceTimer）。 */
	void CleanupThreatList();

	/**
	 * 宽限期到期回调（由 GraceTimer 触发）。
	 * 从威胁列表移除目标；若是当前目标则触发重新评估。
	 */
	void OnGraceExpired(TWeakObjectPtr<AActor> WeakActor);

	// -----------------------------------------------------------------------
	// RadarSensing 事件（必须为 UFUNCTION 才能 AddDynamic）
	// -----------------------------------------------------------------------

	/** 新目标进入感知范围：加入威胁列表或取消宽限；Idle 时立即评估。 */
	UFUNCTION()
	void HandleTargetDetected(AActor* Target);

	/** 目标离开感知范围：启动宽限计时器，不立即重新评估。 */
	UFUNCTION()
	void HandleTargetLost(AActor* Target);

	// -----------------------------------------------------------------------
	// 周期性更新
	// -----------------------------------------------------------------------

	/** 启动（或重启）周期性评估 Timer。 */
	void RestartEvalTimer();

	/**
	 * EvalTimer 回调：
	 *   Idle   → 尝试从威胁列表选目标
	 *   Locked → 兜底检查：目标仍在威胁列表中；绘制 Debug
	 */
	void PeriodicUpdate();

	// -----------------------------------------------------------------------
	// 运行时数据
	// -----------------------------------------------------------------------

	TWeakObjectPtr<AActor>     CurrentTarget;
	FTimerHandle               EvalTimerHandle;
	FMFThreatConfig            Config;
	TArray<FMFThreatRecord>    ThreatRecords;

	UPROPERTY()
	TObjectPtr<UMFRadarSensingComponent> RadarComp;

	// -----------------------------------------------------------------------
	// Debug（CVar: MF.Debug.ThreatSystem）
	// -----------------------------------------------------------------------

	void DrawDebugVisualization() const;
};

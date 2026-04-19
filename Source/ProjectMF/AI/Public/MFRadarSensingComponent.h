// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MFRadarSensingComponent.generated.h"

/**
 * 当雷达检测到 / 失去一个目标时广播。
 * @param Target  被感知到或离开范围的 Actor。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMFRadarTargetEvent, AActor*, Target);

/**
 * FMFRadarSensingConfig — 雷达感知的运行时配置。
 *
 * 存储在 UMFSpawnAIConfig DataAsset 中，由 AMFSpawnAIManager 在
 * SpawnSinglePet 阶段调用 UMFRadarSensingComponent::ApplyConfig() 写入。
 * 这样每种宠物类型可以有独立的感知半径和目标过滤标签，
 * 无需修改蓝图 CDO。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFRadarSensingConfig
{
	GENERATED_BODY()

	/** 雷达感知球形半径（世界单位）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radar",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SensingRadius = 1000.f;

	/**
	 * 目标过滤标签：目标 ASC 必须拥有至少一个此集合中的标签。
	 * 典型值：MF.Team.Player（感知玩家阵营）。
	 * 留空则不过滤阵营，感知所有重叠 Pawn。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radar")
	FGameplayTagContainer TargetTags;

	/** 每次扫描的时间间隔（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radar",
		meta = (ClampMin = "0.05", UIMin = "0.05"))
	float ScanInterval = 0.15f;
};

/**
 * UMFRadarSensingComponent — 轻量级球形雷达感知组件。
 *
 * 职责：
 *   - 以固定时间间隔 (ScanInterval) 在 SensingRadius 范围内做球形重叠检测。
 *   - 通过 TargetTags 过滤目标：目标 ASC 必须拥有 TargetTags 中的至少一个标签。
 *     若 TargetTags 为空，则对所有重叠 Pawn 生效（慎用）。
 *   - 目标进入范围时广播 OnTargetDetected；离开范围时广播 OnTargetLost。
 *   - 当前感知列表可通过 GetPerceivedActors() 查询，供 StateTree / 威胁系统使用。
 *
 * 使用方式：
 *   将组件添加到 AMFAICharacter 或其子类。
 *   在编辑器中配置 SensingRadius 和 TargetTags（例如 MF.Team.Player）。
 *   StateTree 或威胁管理器通过 OnTargetDetected / GetPerceivedActors() 获取结果。
 *
 * 设计约束：
 *   - 不做视线遮挡检测（纯球形雷达，适配 2D 俯视游戏）。
 *   - 感知通道默认为 ECC_Pawn，可在 SensingChannel 属性中调整。
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFRadarSensingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFRadarSensingComponent();

	// -----------------------------------------------------------------------
	// 配置属性
	// -----------------------------------------------------------------------

	/** 雷达感知半径（世界单位）。超出此范围的目标不被感知。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radar|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SensingRadius = 1000.f;

	/**
	 * 目标必须拥有（至少一个）的 GameplayTag 集合。
	 * 典型值：MF.Team.Player（感知玩家阵营）或 MF.Team.Enemy（感知敌对阵营）。
	 * 留空则对所有重叠 Pawn 生效（不过滤阵营）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radar|Config")
	FGameplayTagContainer TargetTags;

	/** 每次扫描的时间间隔（秒）。越小响应越快，CPU 开销越高。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radar|Config", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float ScanInterval = 0.15f;

	/** 球形重叠使用的碰撞通道。默认 ECC_Pawn，覆盖所有标准角色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radar|Config")
	TEnumAsByte<ECollisionChannel> SensingChannel = ECC_Pawn;

	// -----------------------------------------------------------------------
	// 事件委托
	// -----------------------------------------------------------------------

	/** 当一个新目标进入感知范围时触发。每个目标只触发一次（直到再次离开）。 */
	UPROPERTY(BlueprintAssignable, Category = "Radar|Events")
	FMFRadarTargetEvent OnTargetDetected;

	/** 当一个已感知目标离开感知范围或消亡时触发。 */
	UPROPERTY(BlueprintAssignable, Category = "Radar|Events")
	FMFRadarTargetEvent OnTargetLost;

	// -----------------------------------------------------------------------
	// 查询接口
	// -----------------------------------------------------------------------

	/** 返回当前所有在感知范围内的目标（已过滤无效 Actor）。 */
	UFUNCTION(BlueprintPure, Category = "Radar")
	TArray<AActor*> GetPerceivedActors() const;

	/** 指定 Actor 当前是否在感知范围内。 */
	UFUNCTION(BlueprintPure, Category = "Radar")
	bool IsActorPerceived(AActor* Actor) const;

	/** 返回当前感知到的目标数量。 */
	UFUNCTION(BlueprintPure, Category = "Radar")
	int32 GetPerceivedCount() const;

	// -----------------------------------------------------------------------
	// 运行时控制
	// -----------------------------------------------------------------------

	/**
	 * 将 DataAsset 中的配置写入组件并重启定时扫描。
	 * 由 AMFSpawnAIManager::SpawnSinglePet 在 Spawn 完成后调用。
	 * 可在 BeginPlay 之后的任何时刻调用（会重启 Timer）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Radar")
	void ApplyConfig(const FMFRadarSensingConfig& InConfig);

	/** 手动触发一次扫描（不受 ScanInterval 限制）。可用于事件驱动的即时感知。 */
	UFUNCTION(BlueprintCallable, Category = "Radar")
	void ForceScan();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 执行一次球形重叠扫描，更新 PerceivedActors 并广播相应事件。 */
	void PerformScan();

	/**
	 * 检查候选 Actor 是否通过 TargetTags 过滤。
	 * 若 TargetTags 为空，任何非 Owner 的 Actor 均通过。
	 * 否则查询目标 ASC 拥有的标签，至少匹配一个才通过。
	 */
	bool PassesTagFilter(AActor* Candidate) const;

	/**
	 * 在 CVar MF.Debug.RadarSensing 开启时，绘制感知范围球体和目标→AI 箭头。
	 * 仅在非 Shipping 构建（ENABLE_DRAW_DEBUG）中编译。
	 */
	void DrawDebugVisualization(const FVector& OwnerLocation) const;

	/** 定时扫描句柄。 */
	FTimerHandle ScanTimerHandle;

	/**
	 * 当前感知到的 Actor 集合（弱引用，防止持有引用阻止 GC）。
	 * 使用 TSet 保证 O(1) 查找和无重复项。
	 */
	TSet<TWeakObjectPtr<AActor>> PerceivedActors;
};

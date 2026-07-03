// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MFLootTypes.h"
#include "MFLootSubsystem.generated.h"

class UMFLootTable;
class AMFLootPickup;

/**
 * UMFLootSubsystem — 掉落物生成子系统（World Subsystem）。
 *
 * 单管线原则：怪物死亡、采集产出、（未来）任务奖励，一律走
 *   DropFromTable(表, 位置) → roll → 生成 AMFLootPickup 散开落地 → 玩家靠近自动吸附入包。
 *
 * Pickup 类与散开半径在 UMFLootSettings（Project Settings → Game → MF Loot）配置。
 */
UCLASS()
class PROJECTMF_API UMFLootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** roll 一张掉落表：每条独立判定，同 ItemID 合并。表为空/无命中返回空数组。 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<FMFLootResult> RollTable(const UMFLootTable* Table) const;

	/** 在 Location 周围生成掉落物：每个 Result 一个 Pickup，落点在 ScatterRadius 内随机散开并对齐地面。 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SpawnLoot(const TArray<FMFLootResult>& Results, const FVector& Location);

	/** 组合入口：Roll + Spawn。死亡掉落 / 采集产出只需调这一个。 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void DropFromTable(const UMFLootTable* Table, const FVector& Location);

private:
	/** 解析 Settings 里配置的 Pickup 类（软引用同步加载后缓存）。未配置回退 C++ 基类并警告一次。 */
	TSubclassOf<AMFLootPickup> ResolvePickupClass();

	/** 散开落点：XY 随机偏移后向下 trace 对齐地面；trace 不中回退原 Z。 */
	FVector ResolveLandLocation(const FVector& Origin, float ScatterRadius) const;

	UPROPERTY()
	TSubclassOf<AMFLootPickup> CachedPickupClass;

	/** 未配置 PickupClass 的警告只打一次。 */
	bool bWarnedNoPickupClass = false;
};

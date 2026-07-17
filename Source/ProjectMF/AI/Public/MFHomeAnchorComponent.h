// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MFHomeAnchorComponent.generated.h"

// ============================================================
// 配置结构体
// ============================================================

/**
 * FMFHomeAnchorConfig — 出生锚点 / 回家配置，存入 UMFPetConfig DataAsset。
 *
 * 模型（见 Docs/HomeAnchor_Leash_Design.md）：战斗 > 回家 > 巡逻。
 *   巡逻在以锚点为心的 WanderRadius 内；离家超此值且无目标 → 走回锚点。
 *   放弃追击纯靠时间（FMFThreatConfig.LockDuration），此处不做空间硬上限。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFHomeAnchorConfig
{
	GENERATED_BODY()

	/** 启用锚点/回家（false = 旧行为：绕当前位置巡逻、不回家）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HomeAnchor")
	bool bEnableHomeAnchor = true;

	/** 游离半径（cm）：巡逻在以锚点为心此半径内；离家超此值且无目标 → 回家。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HomeAnchor", meta = (ClampMin = "100.0"))
	float WanderRadius = 800.f;

	/** 到家容差（cm）：离锚点小于此值算"已到家"，恢复巡逻。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HomeAnchor", meta = (ClampMin = "10.0"))
	float HomeArrivalTolerance = 120.f;
};

// ============================================================
// 组件
// ============================================================

/**
 * UMFHomeAnchorComponent — AI 出生锚点 + 回家/巡逻判定。
 *
 * 出生时（BeginPlay）记录 owner 当前位置为"家"，适配 SpawnManager 生成与关卡手摆两种来源。
 * 供 StateTree 条件/任务查询：离家多远、是否越出游离半径（该回家）、是否已到家。
 *
 * 纯查询式，不驱动状态——StateTree 每帧调 IsBeyondWander/IsAtHome 决定进/出回家状态。
 * 见 Docs/HomeAnchor_Leash_Design.md、Docs/TargetingSystem_Current.md。
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class PROJECTMF_API UMFHomeAnchorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMFHomeAnchorComponent();

	/** 写入运行时配置。由 AMFPetBase::ApplyPetConfig 调用。 */
	UFUNCTION(BlueprintCallable, Category = "HomeAnchor")
	void ApplyConfig(const FMFHomeAnchorConfig& InConfig);

	/** 记录锚点位置（BeginPlay 自动用 owner 当前位置调；也可外部指定）。 */
	UFUNCTION(BlueprintCallable, Category = "HomeAnchor")
	void SetHome(const FVector& Loc);

	UFUNCTION(BlueprintPure, Category = "HomeAnchor")
	FVector GetHomeLocation() const { return HomeLocation; }

	/** owner 当前离家的水平距离（cm，2D 俯视用平面距离）。未记家返回 0。 */
	UFUNCTION(BlueprintPure, Category = "HomeAnchor")
	float GetDistanceFromHome() const;

	/** owner 离家 > WanderRadius（该回家）。未启用/未记家返回 false。 */
	UFUNCTION(BlueprintPure, Category = "HomeAnchor")
	bool IsBeyondWander() const;

	/** owner 离家 < HomeArrivalTolerance（已到家）。未记家视为在家（不触发回家）。 */
	UFUNCTION(BlueprintPure, Category = "HomeAnchor")
	bool IsAtHome() const;

	/** 启用且已记家。 */
	UFUNCTION(BlueprintPure, Category = "HomeAnchor")
	bool IsEnabled() const { return Config.bEnableHomeAnchor && bHomeSet; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	FVector HomeLocation = FVector::ZeroVector;
	bool    bHomeSet = false;
	FMFHomeAnchorConfig Config;
};

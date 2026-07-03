// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFSceneActorBase.h"
#include "MFGatherNode.generated.h"

class UBoxComponent;
class UMFLootTable;

/**
 * AMFGatherNode — 可采集物（树木/矿石等），落地 AMFSceneActorBase 注释里预留的 TODO。
 *
 * 交互（MVP 最简，不依赖任务/交互底层）：
 *   玩家站进 InteractRadius 内按住采集键 → 现有输入链激活 GA_Pick（本类零改动 GA）
 *   → 本 Actor 检测玩家身上的 MF.GameplayState.Picking tag 累计读条
 *   → 读满 HarvestDuration 产出一次（走 UMFLootSubsystem 掉落管线，落脚下被自动吸走）
 *   → 剩余次数耗尽 → OnDepleted（BP 换残桩外观）/ 直接销毁。
 *
 * 外观：BP 子类配置 Flipbook；每次采集后可在 OnHarvested 里换阶段外观。
 * 扩展位：Gather() 公开入口供未来派宠劳作直接触发一次采集。
 */
UCLASS(Blueprintable)
class PROJECTMF_API AMFGatherNode : public AMFSceneActorBase
{
	GENERATED_BODY()

public:
	AMFGatherNode();

	/**
	 * 立即完成一次采集产出（跳过读条）。
	 * 预留给派宠劳作 / 调试；玩家路径走 Tick 读条后内部调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Gather")
	void Gather();

	/** 当前读条进度 0~1（供 BP/UI 显示）。 */
	UFUNCTION(BlueprintPure, Category = "Gather")
	float GetHarvestProgress() const
	{
		return HarvestDuration > 0.f ? FMath::Clamp(Progress / HarvestDuration, 0.f, 1.f) : 0.f;
	}

	UFUNCTION(BlueprintPure, Category = "Gather")
	int32 GetRemainingHarvests() const { return RemainingHarvests; }

	UFUNCTION(BlueprintPure, Category = "Gather")
	bool IsDepleted() const { return RemainingHarvests <= 0; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** BP 钩子：完成一次采集后调用（换阶段外观/播动画）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Gather")
	void OnHarvested(int32 InRemainingHarvests);

	/** BP 钩子：耗尽时调用（bDestroyOnDepleted=false 时换残桩外观用）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Gather")
	void OnDepleted();

	// -----------------------------------------------------------------------
	// 组件
	// -----------------------------------------------------------------------

	/** 阻挡碰撞（树干/矿石体积）。尺寸在 BP 按外观调。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingBox;

	// -----------------------------------------------------------------------
	// 配置
	// -----------------------------------------------------------------------

	/** 每次采集 roll 一次的掉落表。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather")
	TObjectPtr<UMFLootTable> LootTable;

	/** 可采集总次数（如树 ×3）。 */
	UPROPERTY(EditAnywhere, Category = "Gather", meta = (ClampMin = 1))
	int32 MaxHarvestCount = 3;

	/** 单次采集读条时长（秒）。 */
	UPROPERTY(EditAnywhere, Category = "Gather", meta = (ClampMin = "0.1"))
	float HarvestDuration = 2.f;

	/** 玩家可采集的距离（到本 Actor 中心，cm）。 */
	UPROPERTY(EditAnywhere, Category = "Gather", meta = (ClampMin = "50.0"))
	float InteractRadius = 180.f;

	/** 中断（松开采集键/走出范围）时是否保留读条进度。 */
	UPROPERTY(EditAnywhere, Category = "Gather")
	bool bKeepProgressOnInterrupt = false;

	/** 耗尽后销毁 Actor；false = 留在场景（BP 在 OnDepleted 换残桩外观）。 */
	UPROPERTY(EditAnywhere, Category = "Gather")
	bool bDestroyOnDepleted = true;

private:
	int32 RemainingHarvests = 0;
	float Progress = 0.f;

	/** 玩家在范围内且正持有 MF.GameplayState.Picking。 */
	bool IsPlayerGathering() const;

	/** Debug：读条期间头顶画进度条（MVP 用，正式 UMG 后补）。 */
	void DrawProgressBar() const;
};

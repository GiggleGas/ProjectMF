// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFItemTypes.h"   // FMFPetInstance
#include "MFMetaTypes.generated.h"

/**
 * FMFExtractionResult —— 局内 → 局外的一次性交接数据（瞬态，不入盘）。
 *
 * 由地牢侧 CollectSurvivorsAndSubmit 在「撤离 / 通关 / 全灭」时填充，经
 * UMFMetaSubsystem::SubmitExtraction 写入 PendingExtraction；回主菜单时
 * ReconcileExtraction 消费它并入存档后清空。
 *
 * 归属判定（带入 vs 新抓）由 MetaSubsystem.BroughtInIDs 决定，不在此结构内重复。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFExtractionResult
{
	GENERATED_BODY()

	/** 是否通关（杀第 3 层 Boss=true；主动撤离=false）。决定是否发放复活道具。 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	bool bVictory = false;

	/**
	 * 带出的所有存活宠物（带入老宠 + 本局新抓，无论是否在出战位）。
	 * Reconcile 按 InstanceID 匹配：已在 roster → 刷新快照；否则新增（新抓宠入库）。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	TArray<FMFPetInstance> SurvivingPets;

	/**
	 * 阵亡的「带入老宠」—— Reconcile 时移入墓碑（LostPets），可被复活道具救回。
	 * 新抓宠阵亡不在此列（直接永久消失，不带回）。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	TArray<FMFPetInstance> DeadBroughtInPets;
};

/**
 * FMFRunSnapshot —— 一趟地牢运行的中途快照，用于退出后「继续」续局（入盘）。
 *
 * 落盘时机（见策划案 5.1）：每房间清空 / 主动保存 / 阵亡瞬间。
 * run 结束（撤离/通关/全灭结算）随存档 ClearRunSnapshot 清空。
 *
 * ⚠️ 布局 / 进度类字段强依赖地牢状态模型（同事侧）。此处先放已知字段 + 占位；
 *    地牢骨架到位后补全（当前房间索引 / 已清房间集合 / 布局描述等），方能完整续局。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFRunSnapshot
{
	GENERATED_BODY()

	/** 地牢生成随机种子 —— 续局时据此重建同一座地牢布局。 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta", SaveGame)
	int32 Seed = 0;

	/** 当前所在层（1-3）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta", SaveGame)
	int32 CurrentFloor = 1;

	/** 局内背包快照：本局所有宠物（带入存活/阵亡态 + 新抓），续局时还原。 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta", SaveGame)
	TArray<FMFPetInstance> InRunPets;

	/** 哪些 InstanceID 是「带入」的（区分带入 vs 新抓，决定阵亡去向）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Meta", SaveGame)
	TArray<FGuid> BroughtInIDs;

	// 🔌 TODO(地牢)：当前房间索引 / 已清房间集合 / 房间布局描述等 —— 待地牢状态模型确定后补全。
};

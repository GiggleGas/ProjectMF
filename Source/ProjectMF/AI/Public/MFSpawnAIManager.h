// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "MFSpawnAIConfig.h"
#include "MFSpawnAIManager.generated.h"

class AMFPetBase;
class AMFPetAIController;
class UEnvQuery;

// ============================================================
// 选点规则枚举
// ============================================================

UENUM(BlueprintType)
enum class EMFSpawnPointRule : uint8
{
	/** 在以 Manager 为圆心的环形区域内随机选 NavMesh 可达点（同步）。 */
	NavMeshRandom     UMETA(DisplayName = "NavMesh Random"),

	/** 使用 EQS 查询选点，结果按分数排序后取前 N 个（异步）。 */
	EQSQuery          UMETA(DisplayName = "EQS Query"),

	/**
	 * 直接引用场景中手动摆放的 Actor 作为生成点（同步）。
	 * 将目标 Actor 拖入 SpawnPoints 数组，数量不足时按实际点数生成。
	 */
	ManualSpawnPoints UMETA(DisplayName = "Manual Spawn Points"),
};

// ============================================================
// 单条生成配置（Manager 上的 Struct）
// ============================================================

/**
 * FMFSpawnEntry — 关卡设计师在 AMFSpawnAIManager 上配置的每一组生成规则。
 *
 * 决定"生多少"和"怎么选点"；"生什么"由 Config 资产决定。
 * 一个 Manager 可以有多条 Entry，分别生成不同种类的宠物。
 */
USTRUCT(BlueprintType)
struct FMFSpawnEntry
{
	GENERATED_BODY()

	// ----------------------------------------------------------------
	// 生什么（引用 DataAsset）
	// ----------------------------------------------------------------

	/** 宠物配置资产（PetClass + StateTree + ControllerClass）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UMFSpawnAIConfig> Config;

	// ----------------------------------------------------------------
	// 生多少
	// ----------------------------------------------------------------

	/** 本组生成数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
	          meta = (ClampMin = 1, ClampMax = 50))
	int32 SpawnCount = 3;

	// ----------------------------------------------------------------
	// 怎么选点
	// ----------------------------------------------------------------

	/** 选点规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn|PointRule")
	EMFSpawnPointRule SpawnPointRule = EMFSpawnPointRule::NavMeshRandom;

	// --- NavMeshRandom ---
	/** 生成点离 Manager 的最小距离（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn|PointRule",
	          meta = (EditCondition = "SpawnPointRule == EMFSpawnPointRule::NavMeshRandom",
	                  EditConditionHides, ClampMin = 0.f))
	float MinSpawnRadius = 200.f;

	/** 生成点离 Manager 的最大距离（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn|PointRule",
	          meta = (EditCondition = "SpawnPointRule == EMFSpawnPointRule::NavMeshRandom",
	                  EditConditionHides, ClampMin = 0.f))
	float MaxSpawnRadius = 1500.f;

	// --- EQSQuery ---
	/** EQS 查询资产；结果按分数排序，取前 SpawnCount 个点。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn|PointRule",
	          meta = (EditCondition = "SpawnPointRule == EMFSpawnPointRule::EQSQuery",
	                  EditConditionHides))
	TObjectPtr<UEnvQuery> EQSQuery;

	// --- ManualSpawnPoints ---
	/**
	 * 手动指定的生成点 Actor（从场景大纲拖入）。
	 * 取其 WorldLocation 作为生成位置，顺序取前 SpawnCount 个。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn|PointRule",
	          meta = (EditCondition = "SpawnPointRule == EMFSpawnPointRule::ManualSpawnPoints",
	                  EditConditionHides))
	TArray<TObjectPtr<AActor>> SpawnPoints;
};

// ============================================================
// Manager Actor
// ============================================================

/**
 * AMFSpawnAIManager — 关卡放置的宠物生成管理器。
 *
 * 使用方式：
 *   1. 在关卡中放置 BP_SpawnAIManager（继承本类的蓝图）。
 *   2. 在 Details 面板的 SpawnEntries 中添加若干条 FMFSpawnEntry。
 *      每条 Entry 引用一个 UMFSpawnAIConfig（决定"生什么"），
 *      并自行配置数量和选点规则。
 *   3. Play → BeginPlay 自动执行所有 Entry 的生成流程。
 *
 * 生成流程（每条 Entry）：
 *   选点 → SpawnActor<PetClass> → 确认 Controller → RunStateTree
 *
 * 注意：EQS 规则为异步，其 Entry 的生成会在查询回调后执行。
 */
UCLASS()
class PROJECTMF_API AMFSpawnAIManager : public AActor
{
	GENERATED_BODY()

public:
	AMFSpawnAIManager();

	// ----------------------------------------------------------------
	// 配置（关卡编辑器编辑）
	// ----------------------------------------------------------------

	/** 生成规则列表，每条 Entry 独立处理。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnAI")
	TArray<FMFSpawnEntry> SpawnEntries;

	/** NavMesh 投影时的查询容差（XY 宽松，Z 大以应对坡面）。 */
	UPROPERTY(EditAnywhere, Category = "SpawnAI|Nav")
	FVector NavQueryExtent = FVector(50.f, 50.f, 250.f);

	/** NavMesh 单点查询失败时的最大重试次数。 */
	UPROPERTY(EditAnywhere, Category = "SpawnAI|Nav", meta = (ClampMin = 1))
	int32 NavQueryRetries = 5;

protected:
	virtual void BeginPlay() override;

private:
	// ----------------------------------------------------------------
	// 主流程
	// ----------------------------------------------------------------

	/** 遍历所有 Entry，按规则分发到各选点方法。 */
	void RunSpawnPass();

	/** 处理单条 Entry：选点并调用 SpawnGroup。 */
	void ProcessEntry(const FMFSpawnEntry& Entry);

	/** 批量生成一组宠物，取 Points 前 SpawnCount 个位置。 */
	void SpawnGroup(const FMFSpawnEntry& Entry, TArray<FVector>& Points);

	/** 在指定位置生成一只宠物 + Controller，并启动 StateTree。 */
	void SpawnSinglePet(const FMFSpawnEntry& Entry, const FVector& SpawnLocation);

	// ----------------------------------------------------------------
	// 选点实现
	// ----------------------------------------------------------------

	/** NavMeshRandom：环形随机 + NavMesh 投影（同步）。 */
	TArray<FVector> CollectPoints_NavMeshRandom(const FMFSpawnEntry& Entry) const;

	/** EQSQuery：发起异步查询，结果在 OnEQSQueryFinished 中处理。 */
	void CollectPoints_EQS(const FMFSpawnEntry& Entry);

	/** ManualSpawnPoints：直接从 Entry.SpawnPoints 中取世界坐标（同步）。 */
	TArray<FVector> CollectPoints_Manual(const FMFSpawnEntry& Entry) const;

	// ----------------------------------------------------------------
	// EQS 回调
	// ----------------------------------------------------------------

	/** EQS 查询完成后的回调，QueryID 用于匹配 PendingEQSEntries。 */
	void OnEQSQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	// ----------------------------------------------------------------
	// 运行时状态
	// ----------------------------------------------------------------

	/** 已生成的 Pet，供后续管理（Respawn / 清理）。 */
	UPROPERTY()
	TArray<TObjectPtr<AMFPetBase>> SpawnedPets;

	/**
	 * 待完成的 EQS 查询映射（QueryID → Entry 副本）。
	 * 异步回调到来时据此找到对应 Entry 继续执行生成。
	 */
	TMap<int32, FMFSpawnEntry> PendingEQSEntries;
};

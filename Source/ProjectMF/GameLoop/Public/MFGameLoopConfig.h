// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFGameLoopConfig.generated.h"

class AMFAICharacter;
class AMFPetAIController;
class UStateTree;

/**
 * FMFBossSpawnConfig — Boss 生成配置，内嵌于 UMFGameLoopConfig。
 * 决定生成哪个 Boss、使用哪个 AI 行为、以及在玩家周围多远的位置生成。
 */
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFBossSpawnConfig
{
	GENERATED_BODY()

	/** Boss 蓝图类（必须继承 AMFAICharacter）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss")
	TSubclassOf<AMFAICharacter> BossClass;

	/** Boss 行为的 StateTree 资产。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UStateTree> BossStateTree;

	/** Boss 使用的 AIController 类。留空则使用 AMFPetAIController。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss")
	TSubclassOf<AMFPetAIController> BossControllerClass;

	/** Boss 距玩家的生成距离（cm）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = 100.f))
	float SpawnRadius = 300.f;

	/** Boss 的雷达感知配置（用于检测玩家，TargetTags 通常设为 MF.Team.Player）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|AI")
	FMFRadarSensingConfig RadarConfig;

	/** Boss 的索敌配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|AI")
	FMFThreatConfig ThreatConfig;
};

/**
 * UMFGameLoopConfig — 游戏大循环配置（DataAsset）。
 *
 * 在 B_MFGameMode 的 Details 面板中赋值给 AMFGameMode::GameLoopConfig。
 * 所有关卡节奏参数（捕宠时长、宠物触发数、Boss 配置、围墙规模）均集中于此，
 * 无需修改 C++ 代码即可调整关卡体验。
 */
UCLASS(BlueprintType)
class PROJECTMF_API UMFGameLoopConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// 捕宠阶段
	// -----------------------------------------------------------------------

	/** 捕宠阶段持续时间（秒）。倒计时归零后自动触发 Boss 战。默认 3 分钟。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Catching",
		meta = (ClampMin = 10.f))
	float CatchingPhaseDuration = 180.f;

	/**
	 * 玩家持有此数量的宠物后，可手动提前触发 Boss 战（按 StartBossBattle 键）。
	 * 满足条件时广播 OnBossPhaseReady，供 UI 显示提示。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Catching",
		meta = (ClampMin = 1, ClampMax = 5))
	int32 PetsRequiredForEarlyTrigger = 3;

	// -----------------------------------------------------------------------
	// Boss 配置
	// -----------------------------------------------------------------------

	/** Boss 生成与 AI 配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Boss")
	FMFBossSpawnConfig BossSpawnConfig;

	/**
	 * Boss 战开始时，应用到所有已召唤宠物的雷达感知配置。
	 * TargetTags 应设为 MF.Team.Enemy，使宠物能感知并追击 Boss。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Boss")
	FMFRadarSensingConfig SummonedPetBossRadarConfig;

	// -----------------------------------------------------------------------
	// 竞技场围墙
	// -----------------------------------------------------------------------

	/** 围墙单元蓝图类（需包含阻挡碰撞的 StaticMesh 或 BoxComponent）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Arena")
	TSubclassOf<AActor> ArenaBarrierClass;

	/** 围墙圆圈半径（cm），以玩家位置为圆心。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Arena",
		meta = (ClampMin = 100.f))
	float ArenaRadius = 600.f;

	/** 均匀分布于圆周的围墙片段数量。越多则围墙越密，间隙越小。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop|Arena",
		meta = (ClampMin = 4, ClampMax = 64))
	int32 ArenaBarrierCount = 16;
};

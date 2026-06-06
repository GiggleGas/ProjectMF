// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MFGameLoopConfig.h"
#include "MFGameMode.generated.h"

class AMFAICharacter;
class AMFCharacter;
class AMFPetAIController;
class UMFAttributeSetBase;

// -----------------------------------------------------------------------
// 游戏阶段枚举
// -----------------------------------------------------------------------

UENUM(BlueprintType)
enum class EMFGamePhase : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Catching    UMETA(DisplayName = "Catching"),
	Boss        UMETA(DisplayName = "Boss"),
	Victory     UMETA(DisplayName = "Victory"),
	Defeat      UMETA(DisplayName = "Defeat"),
};

// -----------------------------------------------------------------------
// 事件委托（供 UI Widget / Blueprint 订阅）
// -----------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMFPhaseChanged,   EMFGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMFCountdownTick,  float,        TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMFBossPhaseReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMFGameResult,     bool,         bVictory);

/**
 * AMFGameMode — ProjectMF C++ GameMode 基类。
 *
 * 负责整个游戏循环的权威逻辑。B_MFGameMode 的 Parent Class 设为本类，
 * 在 Details → GameLoop → GameLoopConfig 赋值 DataAsset 即可驱动完整流程。
 *
 * 内部用 M1 区块标注 M1 最小可玩循环的专属实现（捕宠→Boss战→胜负判定）。
 * 未来版本可整体替换或扩展该区块，不影响公共接口。
 */
UCLASS()
class PROJECTMF_API AMFGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// 配置（在 B_MFGameMode Details 中赋值）
	// -----------------------------------------------------------------------

	/** 游戏循环配置 DataAsset。控制捕宠时长、Boss 参数、围墙规模等所有可调节奏。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameLoop")
	TObjectPtr<UMFGameLoopConfig> GameLoopConfig;

	// -----------------------------------------------------------------------
	// 公共接口（由输入层调用，如 AMFCharacter::HandleStartBossBattle）
	// -----------------------------------------------------------------------

	/**
	 * 玩家请求提前触发 Boss 战。
	 * 仅在 Catching 阶段且宠物数量满足条件时生效。
	 */
	UFUNCTION(BlueprintCallable, Category = "GameLoop")
	void RequestBossPhase();

	// -----------------------------------------------------------------------
	// 调试便捷接口。供结算 UI 的"重开 / 退出"按钮调用。
	// 函数体用 !UE_BUILD_SHIPPING 包裹：非 Shipping 正常工作，Shipping 下为空操作。
	// （UFUNCTION 声明不能放进预处理块，故声明常驻、仅函数体按构建裁剪。）
	// -----------------------------------------------------------------------

	/** 重开本局：重新加载当前关卡。 */
	UFUNCTION(BlueprintCallable, Category = "GameLoop|Debug")
	void RestartGame();

	/** 退出游戏（PIE 下结束运行）。 */
	UFUNCTION(BlueprintCallable, Category = "GameLoop|Debug")
	void QuitGame();

	// -----------------------------------------------------------------------
	// 状态查询（UI 轮询 / 条件判断）
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "GameLoop")
	EMFGamePhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "GameLoop")
	float GetTimeRemaining() const { return TimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "GameLoop")
	bool IsBossPhaseReady() const { return bBossPhaseReady; }

	// -----------------------------------------------------------------------
	// 事件（UI Widget / Blueprint 订阅）
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "GameLoop|Events")
	FOnMFPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "GameLoop|Events")
	FOnMFCountdownTick OnCountdownTick;

	/** 玩家首次满足宠物数量条件时广播，供 UI 显示"按键开始Boss战"提示。 */
	UPROPERTY(BlueprintAssignable, Category = "GameLoop|Events")
	FOnMFBossPhaseReady OnBossPhaseReady;

	UPROPERTY(BlueprintAssignable, Category = "GameLoop|Events")
	FOnMFGameResult OnGameResult;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// ========================================================================
	// M1 Game Loop — 最小可玩循环（捕宠 → Boss战 → 胜负判定）
	// 此区块为 M1 专用实现，未来版本可整体替换，不影响上方公共接口。
	// ========================================================================

	void M1_StartCatchingPhase();
	void M1_StartBossPhase();
	void M1_SpawnArenaBarriers(const FVector& Center);
	AMFAICharacter* M1_SpawnBoss(const FVector& PlayerLocation);
	void M1_HandleVictory();
	void M1_HandleDefeat();
	void M1_SetPhase(EMFGamePhase NewPhase);

	// 倒计时
	void M1_OnCountdownFinished();
	void M1_OnCountdownTickCallback();
	void M1_DrawDebugInfo() const;

	// 订阅管理
	void M1_SubscribeToInventoryChanges(AMFCharacter* Player);
	void M1_SubscribeToPlayerDeath(AMFCharacter* Player);
	void M1_SubscribeToBossDeath(AMFAICharacter* Boss);
	void M1_UnsubscribeAll();

	UFUNCTION()
	void M1_OnPetRosterChanged();
	void M1_OnPlayerDied();
	void M1_OnBossDied();

	// M1 运行时状态
	EMFGamePhase CurrentPhase    = EMFGamePhase::Idle;
	float        TimeRemaining   = 0.f;
	bool         bBossPhaseReady = false;

	FTimerHandle M1_CountdownHandle;
	FTimerHandle M1_TickHandle;

	UPROPERTY() TWeakObjectPtr<AMFCharacter>    M1_CachedPlayer;
	UPROPERTY() TWeakObjectPtr<AMFAICharacter>  M1_SpawnedBoss;
	UPROPERTY() TArray<TObjectPtr<AActor>>      M1_SpawnedBarriers;

	FDelegateHandle M1_PlayerDeathHandle;
	FDelegateHandle M1_BossDeathHandle;

	// ========================================================================
	// M1 End
	// ========================================================================
};

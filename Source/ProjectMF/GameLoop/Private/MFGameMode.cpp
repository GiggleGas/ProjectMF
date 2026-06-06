// Copyright ProjectMF. All Rights Reserved.

#include "MFGameMode.h"
#include "MFCharacter.h"
#include "MFAICharacter.h"
#include "MFPetAIController.h"
#include "MFPetBase.h"
#include "MFInventoryComponent.h"
#include "MFAttributeSetBase.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFFactionStatics.h"
#include "MFLog.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#if !UE_BUILD_SHIPPING
#include "Kismet/KismetSystemLibrary.h"
#endif

// ============================================================
// AGameModeBase 生命周期
// ============================================================

void AMFGameMode::BeginPlay()
{
	Super::BeginPlay();
	M1_StartCatchingPhase();
}

void AMFGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	M1_UnsubscribeAll();
	GetWorldTimerManager().ClearTimer(M1_CountdownHandle);
	GetWorldTimerManager().ClearTimer(M1_TickHandle);
	Super::EndPlay(EndPlayReason);
}

// ============================================================
// 公共接口
// ============================================================

void AMFGameMode::RequestBossPhase()
{
	if (CurrentPhase != EMFGamePhase::Catching || !bBossPhaseReady)
	{
		return;
	}
	M1_StartBossPhase();
}

void AMFGameMode::RestartGame()
{
#if !UE_BUILD_SHIPPING
	// 重新加载当前关卡（最简实现，整局状态全部重置）。
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
#endif
}

void AMFGameMode::QuitGame()
{
#if !UE_BUILD_SHIPPING
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
#endif
}

// ========================================================================
// M1 Game Loop — 最小可玩循环（捕宠 → Boss战 → 胜负判定）
// ========================================================================

void AMFGameMode::M1_StartCatchingPhase()
{
	if (!GameLoopConfig)
	{
		MF_LOG_WARNING(LogMFGameLoop,
			TEXT("AMFGameMode: GameLoopConfig not assigned. "
			     "Assign a UMFGameLoopConfig DataAsset in B_MFGameMode Details → GameLoop."));
		return;
	}

	AMFCharacter* Player = Cast<AMFCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player)
	{
		MF_LOG_ERROR(LogMFGameLoop,
			TEXT("AMFGameMode: Player character not found at BeginPlay."));
		return;
	}

	M1_CachedPlayer = Player;

	// 注入宠物复活读秒时长（宠物阵亡→回背包→读秒→变回可召唤，由背包自行驱动）。
	if (UMFInventoryComponent* Inv = Player->GetInventoryComponent())
	{
		Inv->SetPetReviveDuration(GameLoopConfig->PetReviveDuration);
	}

	M1_SubscribeToInventoryChanges(Player);
	M1_SubscribeToPlayerDeath(Player);   // 全阶段监听玩家死亡（捕宠期死亡同样判负）

	TimeRemaining = GameLoopConfig->CatchingPhaseDuration;
	M1_SetPhase(EMFGamePhase::Catching);

	GetWorldTimerManager().SetTimer(
		M1_CountdownHandle, this, &AMFGameMode::M1_OnCountdownFinished,
		GameLoopConfig->CatchingPhaseDuration, /*bLoop=*/false);

	GetWorldTimerManager().SetTimer(
		M1_TickHandle, this, &AMFGameMode::M1_OnCountdownTickCallback,
		1.f, /*bLoop=*/true, /*FirstDelay=*/1.f);

	MF_LOG(LogMFGameLoop,
		TEXT("AMFGameMode [M1]: Catching phase started. Duration=%.0fs, PetsRequired=%d."),
		GameLoopConfig->CatchingPhaseDuration, GameLoopConfig->PetsRequiredForEarlyTrigger);
}

void AMFGameMode::M1_StartBossPhase()
{
	GetWorldTimerManager().ClearTimer(M1_CountdownHandle);
	GetWorldTimerManager().ClearTimer(M1_TickHandle);

	M1_SetPhase(EMFGamePhase::Boss);

	AMFCharacter* Player = M1_CachedPlayer.Get();
	if (!Player)
	{
		MF_LOG_ERROR(LogMFGameLoop,
			TEXT("AMFGameMode [M1]: Player is null when starting Boss phase."));
		M1_HandleDefeat();
		return;
	}

	const FVector PlayerLocation = Player->GetActorLocation();

	M1_SpawnArenaBarriers(PlayerLocation);

	AMFAICharacter* Boss = M1_SpawnBoss(PlayerLocation);
	if (!Boss)
	{
		MF_LOG_ERROR(LogMFGameLoop, TEXT("AMFGameMode [M1]: Boss spawn failed."));
		return;
	}

	M1_SpawnedBoss = Boss;
	M1_SubscribeToBossDeath(Boss);   // 玩家死亡已在 Catching 阶段订阅，此处仅订阅 Boss

	MF_LOG(LogMFGameLoop, TEXT("AMFGameMode [M1]: Boss phase started."));
}

void AMFGameMode::M1_SpawnArenaBarriers(const FVector& Center)
{
	if (!GameLoopConfig || !GameLoopConfig->ArenaBarrierClass)
	{
		MF_LOG_WARNING(LogMFGameLoop,
			TEXT("AMFGameMode [M1]: ArenaBarrierClass not set, skipping barrier spawn."));
		return;
	}

	const int32 Count  = GameLoopConfig->ArenaBarrierCount;
	const float Radius = GameLoopConfig->ArenaRadius;

	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = (static_cast<float>(i) / Count) * UE_TWO_PI;
		const FVector Pos = Center + FVector(FMath::Cos(Angle) * Radius,
		                                     FMath::Sin(Angle) * Radius, 0.f);
		// 围墙切线朝向，+90° 使碰撞面朝向圆心内侧
		const FRotator Rot(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AActor* Barrier = GetWorld()->SpawnActor<AActor>(
			GameLoopConfig->ArenaBarrierClass, Pos, Rot, Params))
		{
			M1_SpawnedBarriers.Add(Barrier);
		}
	}

	MF_LOG(LogMFGameLoop,
		TEXT("AMFGameMode [M1]: Spawned %d barriers (radius=%.0f)."),
		M1_SpawnedBarriers.Num(), Radius);
}

AMFAICharacter* AMFGameMode::M1_SpawnBoss(const FVector& PlayerLocation)
{
	if (!GameLoopConfig || !GameLoopConfig->BossSpawnConfig.BossClass)
	{
		MF_LOG_ERROR(LogMFGameLoop,
			TEXT("AMFGameMode [M1]: BossClass not set in GameLoopConfig.BossSpawnConfig."));
		return nullptr;
	}

	const FMFBossSpawnConfig& BossCfg = GameLoopConfig->BossSpawnConfig;

	const float   Angle      = FMath::FRandRange(0.f, UE_TWO_PI);
	const FVector BossOffset = FVector(FMath::Cos(Angle) * BossCfg.SpawnRadius,
	                                   FMath::Sin(Angle) * BossCfg.SpawnRadius, 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMFAICharacter* Boss = GetWorld()->SpawnActor<AMFAICharacter>(
		BossCfg.BossClass, PlayerLocation + BossOffset, FRotator::ZeroRotator, Params);

	if (!IsValid(Boss))
	{
		MF_LOG_ERROR(LogMFGameLoop,
			TEXT("AMFGameMode [M1]: SpawnActor<Boss> failed at %s."),
			*(PlayerLocation + BossOffset).ToString());
		return nullptr;
	}

	// StaticClass() 返回 UClass*，与 TSubclassOf 三目运算符有歧义，改用 if-else（同 MFSpawnAIManager）。
	TSubclassOf<AMFPetAIController> CtrlClass = AMFPetAIController::StaticClass();
	if (BossCfg.BossControllerClass) { CtrlClass = BossCfg.BossControllerClass; }

	AMFPetAIController* Controller = Cast<AMFPetAIController>(Boss->GetController());

	if (!Controller || (BossCfg.BossControllerClass && !Controller->IsA(BossCfg.BossControllerClass)))
	{
		if (Controller) { Controller->UnPossess(); Controller->Destroy(); }

		FActorSpawnParameters CtrlParams;
		CtrlParams.Owner = Boss;
		Controller = GetWorld()->SpawnActor<AMFPetAIController>(
			CtrlClass, Boss->GetActorLocation(), FRotator::ZeroRotator, CtrlParams);
		if (Controller) { Controller->Possess(Boss); }
	}

	if (!IsValid(Controller))
	{
		MF_LOG_ERROR(LogMFGameLoop,
			TEXT("AMFGameMode [M1]: Failed to obtain controller for Boss '%s'."),
			*Boss->GetName());
		Boss->Destroy();
		return nullptr;
	}

	Controller->RunStateTree(BossCfg.BossStateTree);

	if (auto* Radar = Boss->FindComponentByClass<UMFRadarSensingComponent>())
		Radar->ApplyConfig(BossCfg.RadarConfig);
	if (auto* Threat = Boss->FindComponentByClass<UMFThreatComponent>())
		Threat->ApplyConfig(BossCfg.ThreatConfig);

	// 出生阵营：Boss 默认中立，在此写入 Boss 阵营标签（通常 MF.Team.Boss）。
	if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
		UMFFactionStatics::SetFaction(BossASC, BossCfg.BossTeamTags);

	MF_LOG(LogMFGameLoop,
		TEXT("AMFGameMode [M1]: Boss '%s' spawned at %s."),
		*Boss->GetName(), *Boss->GetActorLocation().ToString());

	return Boss;
}

void AMFGameMode::M1_HandleVictory()
{
	M1_SetPhase(EMFGamePhase::Victory);
	OnGameResult.Broadcast(true);
	MF_LOG(LogMFGameLoop, TEXT("AMFGameMode [M1]: VICTORY."));
}

void AMFGameMode::M1_HandleDefeat()
{
	M1_SetPhase(EMFGamePhase::Defeat);
	OnGameResult.Broadcast(false);
	MF_LOG(LogMFGameLoop, TEXT("AMFGameMode [M1]: DEFEAT."));
}

void AMFGameMode::M1_SetPhase(EMFGamePhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(NewPhase);
	M1_DrawDebugInfo();
}

void AMFGameMode::M1_OnCountdownFinished()
{
	if (CurrentPhase == EMFGamePhase::Catching)
	{
		MF_LOG(LogMFGameLoop, TEXT("AMFGameMode [M1]: Countdown finished, auto-triggering Boss."));
		M1_StartBossPhase();
	}
}

void AMFGameMode::M1_OnCountdownTickCallback()
{
	TimeRemaining = FMath::Max(0.f, TimeRemaining - 1.f);
	OnCountdownTick.Broadcast(TimeRemaining);
	M1_DrawDebugInfo();
}

void AMFGameMode::M1_DrawDebugInfo() const
{
#if !UE_BUILD_SHIPPING
	if (!GEngine) return;

	auto PhaseStr = [](EMFGamePhase Phase) -> const TCHAR*
	{
		switch (Phase)
		{
		case EMFGamePhase::Idle:     return TEXT("Idle");
		case EMFGamePhase::Catching: return TEXT("Catching");
		case EMFGamePhase::Boss:     return TEXT("Boss");
		case EMFGamePhase::Victory:  return TEXT("Victory");
		case EMFGamePhase::Defeat:   return TEXT("Defeat");
		default:                     return TEXT("?");
		}
	};

	static const int32 KeyPhase = 9901;
	static const int32 KeyTime  = 9902;

	const FColor PhaseColor =
		CurrentPhase == EMFGamePhase::Catching ? FColor::Cyan    :
		CurrentPhase == EMFGamePhase::Boss      ? FColor::Red     :
		CurrentPhase == EMFGamePhase::Victory   ? FColor::Green   :
		CurrentPhase == EMFGamePhase::Defeat    ? FColor::Orange  : FColor::White;

	GEngine->AddOnScreenDebugMessage(KeyPhase, 2.f, PhaseColor,
		FString::Printf(TEXT("[M1] Phase: %s"), PhaseStr(CurrentPhase)));

	if (CurrentPhase == EMFGamePhase::Catching)
	{
		const int32 Min = FMath::FloorToInt(TimeRemaining) / 60;
		const int32 Sec = FMath::FloorToInt(TimeRemaining) % 60;
		GEngine->AddOnScreenDebugMessage(KeyTime, 2.f, FColor::Yellow,
			FString::Printf(TEXT("[M1] Time: %02d:%02d  BossReady: %s"),
				Min, Sec, bBossPhaseReady ? TEXT("YES") : TEXT("no")));
	}
#endif
}

// -----------------------------------------------------------------------
// 订阅管理
// -----------------------------------------------------------------------

void AMFGameMode::M1_SubscribeToInventoryChanges(AMFCharacter* Player)
{
	if (UMFInventoryComponent* Inv = Player->GetInventoryComponent())
	{
		Inv->OnPetRosterChanged.AddDynamic(this, &AMFGameMode::M1_OnPetRosterChanged);
	}
}

void AMFGameMode::M1_SubscribeToPlayerDeath(AMFCharacter* Player)
{
	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		if (const auto* ConstSet = ASC->GetSet<UMFAttributeSetBase>())
		{
			M1_PlayerDeathHandle = const_cast<UMFAttributeSetBase*>(ConstSet)->OnDeath
				.AddUObject(this, &AMFGameMode::M1_OnPlayerDied);
		}
	}
}

void AMFGameMode::M1_SubscribeToBossDeath(AMFAICharacter* Boss)
{
	if (UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent())
	{
		if (const auto* ConstSet = ASC->GetSet<UMFAttributeSetBase>())
		{
			M1_BossDeathHandle = const_cast<UMFAttributeSetBase*>(ConstSet)->OnDeath
				.AddUObject(this, &AMFGameMode::M1_OnBossDied);
		}
	}
}

void AMFGameMode::M1_UnsubscribeAll()
{
	if (M1_CachedPlayer.IsValid())
	{
		if (auto* Inv = M1_CachedPlayer->GetInventoryComponent())
			Inv->OnPetRosterChanged.RemoveDynamic(this, &AMFGameMode::M1_OnPetRosterChanged);

		if (auto* ASC = M1_CachedPlayer->GetAbilitySystemComponent())
			if (const auto* S = ASC->GetSet<UMFAttributeSetBase>())
				const_cast<UMFAttributeSetBase*>(S)->OnDeath.Remove(M1_PlayerDeathHandle);
	}

	if (M1_SpawnedBoss.IsValid())
	{
		if (auto* ASC = M1_SpawnedBoss->GetAbilitySystemComponent())
			if (const auto* S = ASC->GetSet<UMFAttributeSetBase>())
				const_cast<UMFAttributeSetBase*>(S)->OnDeath.Remove(M1_BossDeathHandle);
	}

	M1_PlayerDeathHandle.Reset();
	M1_BossDeathHandle.Reset();
}

void AMFGameMode::M1_OnPetRosterChanged()
{
	if (CurrentPhase != EMFGamePhase::Catching || !M1_CachedPlayer.IsValid() || !GameLoopConfig)
		return;

	const UMFInventoryComponent* Inv = M1_CachedPlayer->GetInventoryComponent();
	if (!Inv) return;

	const int32 PetCount = Inv->GetAllPets().Num();
	const bool  bWas     = bBossPhaseReady;
	bBossPhaseReady      = (PetCount >= GameLoopConfig->PetsRequiredForEarlyTrigger);

	if (bBossPhaseReady && !bWas)
	{
		OnBossPhaseReady.Broadcast();
		MF_LOG(LogMFGameLoop,
			TEXT("AMFGameMode [M1]: Boss phase ready (%d pets)."), PetCount);
	}
}

void AMFGameMode::M1_OnPlayerDied()
{
	// 已结算则忽略（OnDeath 只广播一次，这里再防一层重入）。
	if (CurrentPhase == EMFGamePhase::Victory || CurrentPhase == EMFGamePhase::Defeat)
	{
		return;
	}

	// 任何进行中的阶段（捕宠 / Boss）玩家死亡都判负：禁用输入，保留 Pawn 不销毁。
	if (AMFCharacter* Player = M1_CachedPlayer.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			Player->DisableInput(PC);
		}
	}

	// 进入失败结算（结算 UI 由 OnGameResult / OnPhaseChanged 驱动显示）。
	M1_HandleDefeat();
}

void AMFGameMode::M1_OnBossDied()
{
	if (CurrentPhase == EMFGamePhase::Boss) M1_HandleVictory();
}

// ========================================================================
// M1 End
// ========================================================================

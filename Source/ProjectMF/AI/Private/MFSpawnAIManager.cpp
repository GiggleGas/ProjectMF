// Copyright ProjectMF. All Rights Reserved.

#include "MFSpawnAIManager.h"
#include "MFPetBase.h"
#include "MFPetAIController.h"
#include "MFRadarSensingComponent.h"
#include "MFThreatComponent.h"
#include "MFLog.h"
#include "NavigationSystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"

// ============================================================
// 构造
// ============================================================

AMFSpawnAIManager::AMFSpawnAIManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

// ============================================================
// BeginPlay
// ============================================================

void AMFSpawnAIManager::BeginPlay()
{
	Super::BeginPlay();
	RunSpawnPass();
}

// ============================================================
// 主流程
// ============================================================

void AMFSpawnAIManager::RunSpawnPass()
{
	if (SpawnEntries.IsEmpty())
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: SpawnEntries is empty, nothing to spawn."), *GetName());
		return;
	}

	for (const FMFSpawnEntry& Entry : SpawnEntries)
	{
		ProcessEntry(Entry);
	}
}

void AMFSpawnAIManager::ProcessEntry(const FMFSpawnEntry& Entry)
{
	if (!IsValid(Entry.Config))
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: Entry has no Config, skipping."), *GetName());
		return;
	}
	if (!Entry.Config->PetClass)
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: Config '%s' has no PetClass, skipping."),
		               *GetName(), *Entry.Config->GetName());
		return;
	}

	switch (Entry.SpawnPointRule)
	{
	case EMFSpawnPointRule::NavMeshRandom:
		{
			TArray<FVector> Points = CollectPoints_NavMeshRandom(Entry);
			SpawnGroup(Entry, Points);
			break;
		}
	case EMFSpawnPointRule::EQSQuery:
		{
			// 异步：选点结果由 OnEQSQueryFinished 回调处理
			CollectPoints_EQS(Entry);
			break;
		}
	case EMFSpawnPointRule::ManualSpawnPoints:
		{
			TArray<FVector> Points = CollectPoints_Manual(Entry);
			SpawnGroup(Entry, Points);
			break;
		}
	}
}

void AMFSpawnAIManager::SpawnGroup(const FMFSpawnEntry& Entry, TArray<FVector>& Points)
{
	if (Points.IsEmpty())
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: No spawn points collected for Config '%s'."),
		               *GetName(), *Entry.Config->GetName());
		return;
	}

	const int32 Count = FMath::Min(Entry.SpawnCount, Points.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		SpawnSinglePet(Entry, Points[i]);
	}

	if (Points.Num() < Entry.SpawnCount)
	{
		MF_LOG_WARNING(LogMFSpawnAI,
		               TEXT("%s: Config '%s' requested %d pets but only %d points were available."),
		               *GetName(), *Entry.Config->GetName(), Entry.SpawnCount, Points.Num());
	}
}

// ============================================================
// 单只宠物生成
// ============================================================

void AMFSpawnAIManager::SpawnSinglePet(const FMFSpawnEntry& Entry, const FVector& SpawnLocation)
{
	const UMFSpawnAIConfig* Config = Entry.Config;

	// 1. Spawn Pet
	//    AutoPossessAI = PlacedInWorldOrSpawned（AMFAICharacter 设置）会触发自动 Possess，
	//    使用 AMFPetBase CDO 上的 AIControllerClass（= AMFPetAIController）。
	FActorSpawnParameters PetParams;
	PetParams.SpawnCollisionHandlingOverride =
	    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMFPetBase* Pet = GetWorld()->SpawnActor<AMFPetBase>(
	    Config->PetClass, SpawnLocation, FRotator::ZeroRotator, PetParams);

	if (!IsValid(Pet))
	{
		MF_LOG_ERROR(LogMFSpawnAI, TEXT("%s: SpawnActor failed at %s."),
		             *GetName(), *SpawnLocation.ToString());
		return;
	}

	// 2. 确认 Controller
	//    AutoPossess 已完成，取出当前 Controller 并转型。
	//    若 Config 指定了不同的子类，则替换掉自动生成的那个。
	AMFPetAIController* Controller = Cast<AMFPetAIController>(Pet->GetController());

	if (!Controller)
	{
		// 自动 Possess 的 Controller 类型不符（罕见），尝试手动替换
		if (AController* OldCtrl = Pet->GetController())
		{
			OldCtrl->UnPossess();
			OldCtrl->Destroy();
		}

		// StaticClass() 返回 UClass*，与 TSubclassOf 的三目运算符有歧义，改用 if-else。
		TSubclassOf<AMFPetAIController> CtrlClass = AMFPetAIController::StaticClass();
		if (Config->ControllerClass)
		{
			CtrlClass = Config->ControllerClass;
		}

		FActorSpawnParameters CtrlParams;
		CtrlParams.Owner = Pet;
		Controller = GetWorld()->SpawnActor<AMFPetAIController>(
		    CtrlClass, SpawnLocation, FRotator::ZeroRotator, CtrlParams);

		if (Controller)
		{
			Controller->Possess(Pet);
		}
	}
	else if (Config->ControllerClass && !Controller->IsA(Config->ControllerClass))
	{
		// Config 要求特定子类，但自动 Possess 给的是基类，替换之
		Controller->UnPossess();
		Controller->Destroy();
		Controller = nullptr;

		FActorSpawnParameters CtrlParams;
		CtrlParams.Owner = Pet;
		Controller = GetWorld()->SpawnActor<AMFPetAIController>(
		    Config->ControllerClass, SpawnLocation, FRotator::ZeroRotator, CtrlParams);

		if (Controller)
		{
			Controller->Possess(Pet);
		}
	}

	if (!IsValid(Controller))
	{
		MF_LOG_ERROR(LogMFSpawnAI, TEXT("%s: Failed to obtain AMFPetAIController for '%s'."),
		             *GetName(), *Pet->GetName());
		Pet->Destroy();
		return;
	}

	// 3. 启动 StateTree
	Controller->RunStateTree(Config->StateTreeAsset);

	// 4. 初始化雷达感知配置（先于索敌配置写入）
	//    RadarConfig → SensingRadius / TargetTags / ScanInterval 写入组件。
	if (UMFRadarSensingComponent* RadarComp =
		Pet->FindComponentByClass<UMFRadarSensingComponent>())
	{
		RadarComp->ApplyConfig(Config->RadarConfig);
	}

	// 5. 初始化索敌配置（依赖 RadarConfig 已写入，内部校验 EngagementRadius <= SensingRadius）
	if (UMFThreatComponent* ThreatComp =
		Pet->FindComponentByClass<UMFThreatComponent>())
	{
		ThreatComp->ApplyConfig(Config->ThreatConfig);
	}

	// 7. 记录
	SpawnedPets.Add(Pet);

	MF_LOG(LogMFSpawnAI, TEXT("%s: Spawned '%s' with controller '%s' at %s."),
	       *GetName(), *Pet->GetName(), *Controller->GetName(), *SpawnLocation.ToString());
}

// ============================================================
// 选点：NavMeshRandom
// ============================================================

TArray<FVector> AMFSpawnAIManager::CollectPoints_NavMeshRandom(const FMFSpawnEntry& Entry) const
{
	TArray<FVector> Result;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: NavigationSystemV1 not found."), *GetName());
		return Result;
	}

	const FVector Origin       = GetActorLocation();
	const int32   MaxAttempts  = Entry.SpawnCount * NavQueryRetries;

	for (int32 i = 0; i < MaxAttempts && Result.Num() < Entry.SpawnCount; ++i)
	{
		const float Angle     = FMath::FRandRange(0.f, UE_TWO_PI);
		const float Dist      = FMath::FRandRange(Entry.MinSpawnRadius, Entry.MaxSpawnRadius);
		const FVector Candidate = Origin + FVector(FMath::Cos(Angle) * Dist,
		                                           FMath::Sin(Angle) * Dist, 0.f);
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Candidate, NavLoc, NavQueryExtent))
		{
			Result.Add(NavLoc.Location);
		}
	}

	MF_LOG(LogMFSpawnAI, TEXT("%s: NavMeshRandom collected %d / %d points for Config '%s'."),
	       *GetName(), Result.Num(), Entry.SpawnCount, *Entry.Config->GetName());

	return Result;
}

// ============================================================
// 选点：EQSQuery（异步）
// ============================================================

void AMFSpawnAIManager::CollectPoints_EQS(const FMFSpawnEntry& Entry)
{
	// const TObjectPtr 在 UE5.7 中不能隐式转换到 UObject*，需显式 .Get()。
	if (Entry.EQSQuery == nullptr)
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: EQSQuery is null for Config '%s'."),
		               *GetName(), *Entry.Config->GetName());
		return;
	}

	UEnvQueryManager* EQSMgr = UEnvQueryManager::GetCurrent(this);
	if (!EQSMgr)
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: EnvQueryManager not found."), *GetName());
		return;
	}

	FEnvQueryRequest Request(Entry.EQSQuery, this);
	const int32 QueryID = Request.Execute(
	    EEnvQueryRunMode::AllMatching,
	    FQueryFinishedSignature::CreateUObject(this, &AMFSpawnAIManager::OnEQSQueryFinished));

	if (QueryID != INDEX_NONE)
	{
		PendingEQSEntries.Add(QueryID, Entry);
		MF_LOG(LogMFSpawnAI, TEXT("%s: EQS query #%d started for Config '%s'."),
		       *GetName(), QueryID, *Entry.Config->GetName());
	}
	else
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: EQS query failed to start for Config '%s'."),
		               *GetName(), *Entry.Config->GetName());
	}
}

void AMFSpawnAIManager::OnEQSQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (!Result.IsValid())
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: EQS callback received invalid result."), *GetName());
		return;
	}

	const int32 QueryID = Result->QueryID;
	FMFSpawnEntry* EntryPtr = PendingEQSEntries.Find(QueryID);
	if (!EntryPtr)
	{
		// 不属于本 Manager 的查询（正常情况不会发生）
		return;
	}

	// 取出 Entry 后立即移除，避免重复处理
	FMFSpawnEntry Entry = *EntryPtr;
	PendingEQSEntries.Remove(QueryID);

	if (!Result->IsSuccessful())
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: EQS query #%d failed for Config '%s'."),
		               *GetName(), QueryID, *Entry.Config->GetName());
		return;
	}

	TArray<FVector> Locations;
	Result->GetAllAsLocations(Locations);

	MF_LOG(LogMFSpawnAI, TEXT("%s: EQS query #%d returned %d locations for Config '%s'."),
	       *GetName(), QueryID, Locations.Num(), *Entry.Config->GetName());

	SpawnGroup(Entry, Locations);
}

// ============================================================
// 选点：ManualSpawnPoints
// ============================================================

TArray<FVector> AMFSpawnAIManager::CollectPoints_Manual(const FMFSpawnEntry& Entry) const
{
	TArray<FVector> Result;

	for (const TObjectPtr<AActor>& PointActor : Entry.SpawnPoints)
	{
		if (IsValid(PointActor))
		{
			Result.Add(PointActor->GetActorLocation());
		}
	}

	if (Result.IsEmpty())
	{
		MF_LOG_WARNING(LogMFSpawnAI, TEXT("%s: ManualSpawnPoints is empty for Config '%s'."),
		               *GetName(), *Entry.Config->GetName());
	}

	return Result;
}

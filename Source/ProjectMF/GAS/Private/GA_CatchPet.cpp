// Copyright ProjectMF. All Rights Reserved.

#include "GA_CatchPet.h"
#include "AT_WaitPetTarget.h"
#include "AT_MoveBall.h"
#include "MFCatchPetConfig.h"
#include "MFCatchable.h"
#include "MFCatchRopeActor.h"
#include "MFCatchBallActor.h"
#include "MFPetBase.h"
#include "MFGameplayTags.h"
#include "MFCharacter.h"
#include "MFInventoryComponent.h"
#include "MFLog.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

// ============================================================
// 构造
// ============================================================

UGA_CatchPet::UGA_CatchPet()
{
	AbilityTags.AddTag(MFGameplayTags::Ability_CatchPet);

	// 激活期间阻止同类型 Ability 再次激活（防止重入）
	ActivationBlockedTags.AddTag(MFGameplayTags::Ability_CatchPet);

	// 单次实例化（整个技能生命周期共用一个实例）
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ============================================================
// ActivateAbility
// ============================================================

void UGA_CatchPet::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: ActivateAbility called."));

	// --- 前置校验 ---
	if (!CatchConfig)
	{
		MF_LOG_ERROR(LogMFAbility,
			TEXT("GA_CatchPet: CatchConfig is null! Please assign a UMFCatchPetConfig DA in BP_GA_CatchPet defaults."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		MF_LOG_WARNING(LogMFAbility, TEXT("GA_CatchPet: CommitAbility failed (cost/cooldown check)."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// --- 进入 Phase 1 ---
	EnterPhase1_Aiming();
}

// ============================================================
// EndAbility
// ============================================================

void UGA_CatchPet::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: EndAbility called. Cancelled=%d"), bWasCancelled ? 1 : 0);

	// 清理 Phase 1 Task（若仍活跃）
	if (UAbilityTask_WaitPetTarget* Task = Task_WaitTarget.Get())
	{
		Task->EndTask();
	}
	Task_WaitTarget.Reset();

	// 清理 Phase 3 Task（若仍活跃）
	if (UAbilityTask_MoveBall* Task = Task_MoveBall.Get())
	{
		Task->EndTask();
	}
	Task_MoveBall.Reset();

	// 清理 Phase 2 Actor
	DestroyPhase2Actors();

	// 清除抓宠状态 Tag（保险起见，ActivationOwnedTags 通常会自动移除）
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayTagContainer TagsToRemove;
		TagsToRemove.AddTag(MFGameplayTags::Catching_State_Aiming);
		TagsToRemove.AddTag(MFGameplayTags::Catching_State_RopeActive);
		TagsToRemove.AddTag(MFGameplayTags::Catching_State_WaitingForBounce);
		ASC->RemoveLooseGameplayTags(TagsToRemove);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================
// Phase 1 — 瞄准
// ============================================================

void UGA_CatchPet::EnterPhase1_Aiming()
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Entering Phase 1 — Aiming."));

	// 添加状态 Tag
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MFGameplayTags::Catching_State_Aiming);
	}

	// 创建并启动瞄准 Task
	UAbilityTask_WaitPetTarget* Task = UAbilityTask_WaitPetTarget::CreateWaitPetTarget(
		this,
		FName(TEXT("WaitPetTarget")),
		CatchConfig);

	if (!Task)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: Failed to create WaitPetTarget task!"));
		EndCatch(false);
		return;
	}

	Task->OnTargetFound.AddDynamic(this,     &UGA_CatchPet::OnTargetFound);
	Task->OnTargetLost.AddDynamic(this,      &UGA_CatchPet::OnTargetLost);
	Task->OnTargetConfirmed.AddDynamic(this, &UGA_CatchPet::OnTargetConfirmed);
	Task->OnCancelled.AddDynamic(this,       &UGA_CatchPet::OnAimCancelled);

	Task_WaitTarget = Task;
	Task->ReadyForActivation();

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: WaitPetTarget task started."));
}

// ============================================================
// Phase 2 — 绳索 + 球
// ============================================================

void UGA_CatchPet::EnterPhase2_Rope(AActor* InTarget)
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Entering Phase 2 — RopeActive. Target: %s"),
		InTarget ? *InTarget->GetName() : TEXT("null"));

	if (!IsValid(InTarget))
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: EnterPhase2_Rope — target is invalid!"));
		EndCatch(false);
		return;
	}

	TargetPet = InTarget;

	// 切换状态 Tag
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::Catching_State_Aiming);
		ASC->AddLooseGameplayTag(MFGameplayTags::Catching_State_RopeActive);
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: Avatar actor is null in Phase 2!"));
		EndCatch(false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: World is null in Phase 2!"));
		EndCatch(false);
		return;
	}

	// --- Spawn 绳索 ---
	if (CatchConfig->RopeActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner   = Avatar;
		SpawnParams.Instigator = Cast<APawn>(Avatar);

		ACatchRopeActor* Rope = World->SpawnActor<ACatchRopeActor>(
			CatchConfig->RopeActorClass,
			Avatar->GetActorLocation(),
			FRotator::ZeroRotator,
			SpawnParams);

		if (Rope)
		{
			Rope->SetEndpoints(Avatar, InTarget);
			RopeActor = Rope;
			MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Rope actor spawned successfully."));
		}
		else
		{
			MF_LOG_ERROR(LogMFAbility,
				TEXT("GA_CatchPet: Failed to spawn RopeActor from class %s!"),
				*CatchConfig->RopeActorClass->GetName());
		}
	}
	else
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("GA_CatchPet: RopeActorClass not set in CatchConfig — rope will not be visible."));
	}

	// --- Spawn 球 ---
	if (CatchConfig->BallActorClass)
	{
		FActorSpawnParameters BallSpawnParams;
		BallSpawnParams.Owner      = Avatar;
		BallSpawnParams.Instigator = Cast<APawn>(Avatar);

		const FVector BallStartPos = Avatar->GetActorLocation() + FVector(0.f, 0.f, 50.f);

		ACatchBallActor* Ball = World->SpawnActor<ACatchBallActor>(
			CatchConfig->BallActorClass,
			BallStartPos,
			FRotator::ZeroRotator,
			BallSpawnParams);

		if (Ball)
		{
			BallActor = Ball;
			MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Ball actor spawned at %s."),
				*BallStartPos.ToString());
		}
		else
		{
			MF_LOG_ERROR(LogMFAbility,
				TEXT("GA_CatchPet: Failed to spawn BallActor from class %s!"),
				*CatchConfig->BallActorClass->GetName());
		}
	}
	else
	{
		MF_LOG_WARNING(LogMFAbility,
			TEXT("GA_CatchPet: BallActorClass not set in CatchConfig — ball will not be visible."));
	}

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Phase 2 setup complete — entering Phase 3."));
	EnterPhase3_MoveBall();
}

// ============================================================
// Phase 3 — 弹射球 QTE
// ============================================================

void UGA_CatchPet::EnterPhase3_MoveBall()
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Entering Phase 3 — MoveBall QTE."));

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: EnterPhase3_MoveBall — Avatar is null!"));
		EndCatch(false);
		return;
	}

	ACatchBallActor* Ball = BallActor.Get();
	if (!IsValid(Ball))
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: EnterPhase3_MoveBall — BallActor is invalid!"));
		EndCatch(false);
		return;
	}

	AActor* Pet = TargetPet.Get();
	if (!IsValid(Pet))
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: EnterPhase3_MoveBall — TargetPet is invalid!"));
		EndCatch(false);
		return;
	}

	UAbilityTask_MoveBall* Task = UAbilityTask_MoveBall::CreateMoveBall(
		this,
		FName(TEXT("MoveBall")),
		Ball,
		Avatar,
		Pet,
		CatchConfig);

	if (!Task)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("GA_CatchPet: Failed to create MoveBall task!"));
		EndCatch(false);
		return;
	}

	Task->OnBallReachedPlayer.AddDynamic(this, &UGA_CatchPet::OnBallReachedPlayer);
	Task->OnQTESuccess.AddDynamic(this,        &UGA_CatchPet::OnQTESuccess);
	Task->OnQTEFailed.AddDynamic(this,         &UGA_CatchPet::OnQTEFailed);
	Task->OnAllBouncesComplete.AddDynamic(this, &UGA_CatchPet::OnAllBouncesComplete);

	Task_MoveBall = Task;
	Task->ReadyForActivation();

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: MoveBall task started. MaxBounce=%d, Speed=%.0f, QTELimit=%.1fs."),
		CatchConfig->MaxBounceCount, CatchConfig->BallSpeed, CatchConfig->QTETimeLimit);
}

// ============================================================
// Phase 3 回调
// ============================================================

void UGA_CatchPet::OnBallReachedPlayer()
{
	// QTE 窗口开启 — 添加状态 Tag 供 UI / Blueprint 监听
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MFGameplayTags::Catching_State_WaitingForBounce);
	}

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Ball reached player — QTE window open!"));
}

void UGA_CatchPet::OnQTESuccess()
{
	// QTE 窗口关闭 — 球已反弹，移除等待 Tag
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::Catching_State_WaitingForBounce);
	}

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: QTE success — ball bounced back toward pet."));
}

void UGA_CatchPet::OnQTEFailed()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::Catching_State_WaitingForBounce);
	}

	MF_LOG_WARNING(LogMFAbility, TEXT("GA_CatchPet: QTE failed — catch aborted."));
	EndCatch(false);
}

void UGA_CatchPet::OnAllBouncesComplete()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::Catching_State_WaitingForBounce);
	}

	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: All bounces complete — catch succeeded!"));
	EndCatch(true);
}

// ============================================================
// 收尾
// ============================================================

void UGA_CatchPet::EndCatch(bool bSuccess)
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: EndCatch — Success=%d"), bSuccess ? 1 : 0);

	if (bSuccess)
	{
		if (AActor* Pet = TargetPet.Get())
		{
			if (Pet->GetClass()->ImplementsInterface(UMFCatchable::StaticClass()))
			{
				AActor* Avatar = GetAvatarActorFromActorInfo();

				// 1. 通知宠物被收服（设置 bIsCaught，不再内部 Destroy）
				IMFCatchable::Execute_OnCaught(Pet, Avatar);

				// 2. 序列化当前状态 + 注册到背包
				AMFPetBase* PetBase = Cast<AMFPetBase>(Pet);
				AMFCharacter* Player = Cast<AMFCharacter>(Avatar);
				if (PetBase && Player)
				{
					if (UMFInventoryComponent* Inventory = Player->GetInventoryComponent())
					{
						FMFPetInstance Snapshot;
						PetBase->SerializeToInstance(Snapshot);

						const FGuid NewID = Inventory->RegisterCaughtPet(Snapshot);
						if (NewID.IsValid())
						{
							MF_LOG(LogMFAbility, TEXT("GA_CatchPet: '%s' registered, InstanceID=%s."),
								*PetBase->PetItemID.ToString(), *NewID.ToString());
						}
						else
						{
							MF_LOG_WARNING(LogMFAbility,
								TEXT("GA_CatchPet: RegisterCaughtPet failed for '%s'."),
								*PetBase->PetItemID.ToString());
						}
					}
				}

				// 3. Actor 序列化完毕后销毁
				Pet->Destroy();
			}
			else
			{
				MF_LOG_WARNING(LogMFAbility,
					TEXT("GA_CatchPet: Target %s does not implement IMFCatchable."),
					*Pet->GetName());
			}
		}
	}
	else
	{
		// 通知宠物抓取失败
		if (AActor* Pet = TargetPet.Get())
		{
			if (Pet->GetClass()->ImplementsInterface(UMFCatchable::StaticClass()))
			{
				IMFCatchable::Execute_OnCatchFailed(Pet, GetAvatarActorFromActorInfo());
				MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Called OnCatchFailed on %s."), *Pet->GetName());
			}
		}
	}

	TargetPet.Reset();

	// 结束 Ability（bWasCancelled = !bSuccess）
	const FGameplayAbilitySpecHandle     Handle    = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo*     ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivInfo, true, !bSuccess);
}

void UGA_CatchPet::DestroyPhase2Actors()
{
	if (ACatchRopeActor* Rope = RopeActor.Get())
	{
		MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Destroying rope actor."));
		Rope->Destroy();
	}
	RopeActor.Reset();

	if (ACatchBallActor* Ball = BallActor.Get())
	{
		MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Destroying ball actor."));
		Ball->Destroy();
	}
	BallActor.Reset();
}

// ============================================================
// Task 回调
// ============================================================

void UGA_CatchPet::OnTargetFound(AActor* Target, bool bCanCatch)
{
	// 可在此更新 UI 提示（如显示宠物名称、等级等）
	// 当前仅 Log，不做其他处理（高亮由 Task 管理）
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: OnTargetFound — %s, CanCatch=%d"),
		Target ? *Target->GetName() : TEXT("null"), bCanCatch ? 1 : 0);
}

void UGA_CatchPet::OnTargetLost()
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: OnTargetLost."));
}

void UGA_CatchPet::OnTargetConfirmed(AActor* Target)
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: OnTargetConfirmed — %s. Ending Phase 1 Task."),
		Target ? *Target->GetName() : TEXT("null"));

	// 关闭 Phase 1 Task（内部会清除高亮和鼠标指针）
	if (UAbilityTask_WaitPetTarget* Task = Task_WaitTarget.Get())
	{
		Task->EndTask();
	}
	Task_WaitTarget.Reset();

	// 过渡到 Phase 2
	EnterPhase2_Rope(Target);
}

void UGA_CatchPet::OnAimCancelled()
{
	MF_LOG(LogMFAbility, TEXT("GA_CatchPet: Aim cancelled by player."));
	EndCatch(false);
}

// Copyright ProjectMF. All Rights Reserved.

#include "GA_AIMoveAbilityBase.h"

#include "MFMoveAbilityData.h"
#include "MFCombatStatics.h"
#include "MFGameplayTags.h"
#include "MFAICharacter.h"
#include "MFCharacterBase.h"
#include "MFThreatComponent.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "PaperZDAnimationComponent.h"
#include "PaperZDAnimInstance.h"
#include "AnimSequences/PaperZDAnimSequence.h"

#include "Engine/World.h"

// ============================================================================
// Debug CVar（统管所有移动技能：冲撞 / 跳跃 / …）
// ============================================================================

static TAutoConsoleVariable<int32> CVarMoveDebug(
	TEXT("mf.debug.move"),
	0,
	TEXT("0=off  1=draw move-ability paths, hit detection, and hit/filtered targets (charge/jump/...)"),
	ECVF_Cheat);

// ============================================================================
// Construction
// ============================================================================

UGA_AIMoveAbilityBase::UGA_AIMoveAbilityBase()
{
	// 死亡 / 眩晕期间禁止发起移动技能（与 OnStunnedTagChanged / HandleDeath 的约定一致）。
	// 自身的 Ability tag 由子类构造函数 SetAssetTags 设置。
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Stunned);
}

// ============================================================================
// UGameplayAbility interface
// ============================================================================

void UGA_AIMoveAbilityBase::ActivateAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*            TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UMFMoveAbilityData* Data = GetMoveData();
	if (!Data)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("[GA_Move] %s has no move data asset — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 需要目标但没有目标 → 取消（在持有状态标签前先判，避免加了又立刻移除）。
	if (Data->bRequireTarget && !GetCurrentTarget())
	{
		MF_LOG_WARNING(LogMFAbility, TEXT("[GA_Move] %s has no threat target — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle         = Handle;
	CachedActivationInfo = ActivationInfo;
	HitTargets.Reset();

	// 初始瞄准：激活瞬间锁定一次方向/落点（不跟随时整段前摇都是这个锁定的预警）。
	RefreshAim();

	// 预警：前摇开始即显示（各技能在 Begin/Update/End 里实现形状，默认空=不显示）。
	InitTelegraph();
	BeginTelegraph();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(GetActiveStateTag());
	}

	// 前摇动画：优先 WindupAnim，否则退回 AttackAnim（位移动画）。
	PlayMoveAnim(Data->WindupAnim ? Data->WindupAnim : Data->AttackAnim);

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();

		// 前摇结束 → 开始位移。
		TM.SetTimer(WindupTimer, this, &UGA_AIMoveAbilityBase::OnWindupFinished,
			FMath::Max(Data->WindupSeconds, 0.01f), false);

		// 蓄力跟随：前 FollowDuration 秒内持续重新锁定，之后冻结。
		// FollowDuration 夹紧到 [0, WindupSeconds]，确保跟随一定在前摇内结束。
		const float FollowTime = FMath::Clamp(Data->FollowDuration, 0.f, Data->WindupSeconds);
		if (Data->bFollowTarget && FollowTime > 0.f)
		{
			TM.SetTimer(WindupFollowTimer, this, &UGA_AIMoveAbilityBase::WindupFollowTick, MFMove::TickInterval, true);
			TM.SetTimer(FollowStopTimer, this, &UGA_AIMoveAbilityBase::StopWindupFollow, FollowTime, false);
		}
	}
	// GA 保持 Running 直到 FinishMove / 被打断。
}

void UGA_AIMoveAbilityBase::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	ClearAllTimers();

	// 安全兜底：被打断（眩晕/死亡）时可能尚未走到 StartRecovery，确保预警被清掉（幂等）。
	EndTelegraph();

	// 被打断（眩晕/死亡）时可能停在位移中途——确保移动模式归还。
	RestoreMovement();

	// 停掉位移动画 override，让 locomotion 状态机接管——技能时长与动画时长解耦
	// （尤其位移动画常是循环的），不能依赖动画自己播完回到 locomotion。
	StopMoveAnim();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(GetActiveStateTag());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================================
// Windup → Movement handoff
// ============================================================================

void UGA_AIMoveAbilityBase::OnWindupFinished()
{
	// 显式收尾蓄力跟随（FollowDuration 与 WindupSeconds 相等时两定时器同帧触发，顺序不定）。
	StopWindupFollow();

	UMFMoveAbilityData* Data = GetMoveData();
	if (!GetMFCharacter() || !Data)
	{
		FinishMove();
		return;
	}

	PlayMoveAnim(Data->AttackAnim); // 切到位移（冲刺/起跳）动画
	TakeOverMovement();             // MOVE_None，逐帧 SetActorLocation
	BeginMovement();                // ← 子类锁定专有参数 + 启动 MotionTimer
}

void UGA_AIMoveAbilityBase::WindupFollowTick()
{
	RefreshAim();
	UpdateTelegraph(); // 跟随期：预警随重新锁定的方向/落点移动
}

void UGA_AIMoveAbilityBase::StopWindupFollow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WindupFollowTimer);
	}
}

void UGA_AIMoveAbilityBase::RefreshAim()
{
	const AActor* Target = GetCurrentTarget();
	bAimHasTarget = (Target != nullptr);
	if (Target)
	{
		AimTargetLocation = Target->GetActorLocation();
	}
	AimDirection = ComputeAimDirection();
}

// ============================================================================
// Recovery / finish
// ============================================================================

void UGA_AIMoveAbilityBase::StartRecovery()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MotionTimer);
	}

	// 位移结束（命中/落地）→ 撤掉预警。
	EndTelegraph();

	RestoreMovement();

	UWorld* World = GetWorld();
	if (World)
	{
		const UMFMoveAbilityData* Data = GetMoveData();
		const float Recovery = Data ? FMath::Max(Data->RecoverySeconds, 0.01f) : 0.01f;
		World->GetTimerManager().SetTimer(RecoveryTimer, this, &UGA_AIMoveAbilityBase::FinishMove, Recovery, false);
	}
	else
	{
		FinishMove();
	}
}

void UGA_AIMoveAbilityBase::FinishMove()
{
	EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
}

void UGA_AIMoveAbilityBase::ClearAllTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(WindupTimer);
		TM.ClearTimer(WindupFollowTimer);
		TM.ClearTimer(FollowStopTimer);
		TM.ClearTimer(MotionTimer);
		TM.ClearTimer(RecoveryTimer);
	}
}

// ============================================================================
// Movement takeover
// ============================================================================

void UGA_AIMoveAbilityBase::TakeOverMovement()
{
	if (AMFCharacterBase* Char = GetMFCharacter())
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->StopMovementImmediately();
			CMC->SetMovementMode(MOVE_None);
		}
	}
}

void UGA_AIMoveAbilityBase::RestoreMovement()
{
	AMFCharacterBase* Char = GetMFCharacter();
	if (!Char) return;

	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
	if (!CMC || CMC->MovementMode != MOVE_None) return;

	// 仍处于眩晕 / 死亡时不抢着恢复行走——由 OnStunnedTagChanged / 死亡流程各自负责。
	// 镜像 AMFCharacterBase::OnStunnedTagChanged 的判定。
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bBlocked = ASC &&
		(ASC->HasMatchingGameplayTag(MFGameplayTags::State_Stunned) ||
		 ASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead));
	if (!bBlocked)
	{
		CMC->SetMovementMode(MOVE_Walking);
	}
}

// ============================================================================
// Shared combat / helpers
// ============================================================================

bool UGA_AIMoveAbilityBase::ApplyHitToTarget(AActor* Target)
{
	if (!Target) return false;

	bool bAlreadyHit = false;
	HitTargets.Add(Target, &bAlreadyHit);
	if (bAlreadyHit) return false;

	const UMFMoveAbilityData* Data = GetMoveData();
	if (!Data) return true;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	UMFCombatStatics::ApplyDamage(SourceASC, TargetASC, Data->DamageGE, Data->DamageMultiplier);
	UMFCombatStatics::ApplyOnHitEffects(SourceASC, TargetASC, Data->OnHitEffects, GetAbilityLevel());
	return true;
}

void UGA_AIMoveAbilityBase::SpawnImpactArea(const FVector& Location)
{
	const UMFMoveAbilityData* Data = GetMoveData();
	if (Data && Data->ImpactAreaData)
	{
		if (AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			UMFCombatStatics::SpawnAreaEffect(Avatar, Data->ImpactAreaData, Location);
		}
	}
}

void UGA_AIMoveAbilityBase::PlayMoveAnim(UPaperZDAnimSequence* Anim)
{
	if (!Anim) return;

	if (AMFCharacterBase* Char = GetMFCharacter())
	{
		if (UPaperZDAnimationComponent* AnimComp = Char->FindComponentByClass<UPaperZDAnimationComponent>())
		{
			if (UPaperZDAnimInstance* AnimInst = AnimComp->GetAnimInstance())
			{
				AnimInst->PlayAnimationOverride(Anim);
			}
		}
	}
}

void UGA_AIMoveAbilityBase::StopMoveAnim()
{
	if (AMFCharacterBase* Char = GetMFCharacter())
	{
		if (UPaperZDAnimationComponent* AnimComp = Char->FindComponentByClass<UPaperZDAnimationComponent>())
		{
			if (UPaperZDAnimInstance* AnimInst = AnimComp->GetAnimInstance())
			{
				// 清掉本技能的 override，回到 AnimBP locomotion（UpdateAnimation 按当前速度驱动）。
				AnimInst->StopAllAnimationOverrides();
			}
		}
	}
}

bool UGA_AIMoveAbilityBase::IsMoveDebugEnabled() const
{
	return CVarMoveDebug.GetValueOnGameThread() != 0;
}

FVector UGA_AIMoveAbilityBase::ComputeAimDirection() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FVector::ForwardVector;
	}

	FVector Dir = Avatar->GetActorForwardVector();
	if (const AActor* Target = GetCurrentTarget())
	{
		Dir = Target->GetActorLocation() - Avatar->GetActorLocation();
	}
	Dir.Z = 0.f;

	const FVector Normalized = Dir.GetSafeNormal();
	return Normalized.IsNearlyZero() ? Avatar->GetActorForwardVector().GetSafeNormal2D() : Normalized;
}

AMFAICharacter* UGA_AIMoveAbilityBase::GetAICharacter() const
{
	return Cast<AMFAICharacter>(GetAvatarActorFromActorInfo());
}

AActor* UGA_AIMoveAbilityBase::GetCurrentTarget() const
{
	AMFAICharacter* AI = GetAICharacter();
	if (!AI) return nullptr;

	UMFThreatComponent* ThreatComp = AI->FindComponentByClass<UMFThreatComponent>();
	return ThreatComp ? ThreatComp->GetCurrentTarget() : nullptr;
}

UMFAttackDataBase* UGA_AIMoveAbilityBase::GetAttackDataBase() const
{
	return GetMoveData();
}

// Copyright ProjectMF. All Rights Reserved.

#include "GA_Charge.h"

#include "MFChargeData.h"
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

#include "Kismet/KismetSystemLibrary.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Debug CVar
// ============================================================================

static TAutoConsoleVariable<int32> CVarChargeDebug(
	TEXT("mf.debug.charge"),
	0,
	TEXT("0=off  1=draw charge dash path, hit detection sphere, and hit/filtered targets"),
	ECVF_Cheat);

namespace
{
	/** 冲刺逐帧位移的固定步长（约 60Hz）。用作位置积分的 dt。 */
	constexpr float DashTickInterval = 1.f / 60.f;
}

// ============================================================================
// Construction
// ============================================================================

UGA_Charge::UGA_Charge()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pet_Move_Charge));

	// 死亡 / 眩晕期间禁止发起冲撞（与 OnStunnedTagChanged / HandleDeath 的约定一致）。
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(MFGameplayTags::State_Stunned);
}

// ============================================================================
// UGameplayAbility interface
// ============================================================================

void UGA_Charge::ActivateAbility(
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

	if (!ChargeData)
	{
		MF_LOG_ERROR(LogMFAbility, TEXT("[GA_Charge] ChargeData is null on %s — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 需要目标但没有目标 → 取消（在持有状态标签前先判，避免加了又立刻移除）。
	if (ChargeData->bRequireTarget && !GetCurrentTarget())
	{
		MF_LOG_WARNING(LogMFAbility, TEXT("[GA_Charge] %s has no threat target — ability cancelled."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle         = Handle;
	CachedActivationInfo = ActivationInfo;
	HitTargets.Reset();
	InitialOverlapActors.Reset();
	DistanceTraveled = 0.f;

	// 初始瞄准：激活瞬间锁定一次冲刺方向（不跟随时整段前摇都是这个方向的预警）。
	DashDirection = ComputeAimDirection();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MFGameplayTags::State_Charging);
	}

	MF_LOG(LogMFAbility,
		TEXT("[GA_Charge] %s activated. Windup=%.2fs Speed=%.0f MaxDist=%.0f Radius=%.0f bStopOnHit=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		ChargeData->WindupSeconds, ChargeData->ChargeSpeed, ChargeData->MaxDistance,
		ChargeData->ChargeRadius, ChargeData->bStopOnHit ? 1 : 0);

	// 前摇动画：优先 WindupAnim，否则退回 AttackAnim（冲刺动画）。
	if (UPaperZDAnimSequence* WindupAnim = ChargeData->WindupAnim ? ChargeData->WindupAnim : ChargeData->AttackAnim)
	{
		if (AMFCharacterBase* Char = GetMFCharacter())
		{
			if (UPaperZDAnimationComponent* AnimComp = Char->FindComponentByClass<UPaperZDAnimationComponent>())
			{
				if (UPaperZDAnimInstance* AnimInst = AnimComp->GetAnimInstance())
				{
					AnimInst->PlayAnimationOverride(WindupAnim);
				}
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();

		// 前摇结束后开始冲刺。
		TM.SetTimer(WindupTimer, this, &UGA_Charge::BeginDash,
			FMath::Max(ChargeData->WindupSeconds, 0.01f), false);

		// 蓄力跟随：前摇的前 FollowDuration 秒内持续重新瞄准，之后锁定方向。
		// FollowDuration 夹紧到 [0, WindupSeconds]，确保跟随一定在前摇内结束。
		const float FollowTime = FMath::Clamp(ChargeData->FollowDuration, 0.f, ChargeData->WindupSeconds);
		if (ChargeData->bFollowTarget && FollowTime > 0.f)
		{
			TM.SetTimer(WindupFollowTimer, this, &UGA_Charge::WindupFollowTick, DashTickInterval, true);
			TM.SetTimer(FollowStopTimer, this, &UGA_Charge::StopWindupFollow, FollowTime, false);
		}
	}
	// GA 保持 Running 直到 FinishCharge / 被打断。
}

void UGA_Charge::EndAbility(
	const FGameplayAbilitySpecHandle     Handle,
	const FGameplayAbilityActorInfo*     ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool                                 bReplicateEndAbility,
	bool                                 bWasCancelled)
{
	ClearAllTimers();

	// 被打断（眩晕/死亡）时可能停在冲刺中途——确保移动模式归还。
	RestoreMovement();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MFGameplayTags::State_Charging);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ============================================================================
// Phase progression
// ============================================================================

void UGA_Charge::BeginDash()
{
	// 确保蓄力跟随已停止——此刻方向锁定（FollowDuration 与 WindupSeconds 相等时两定时器
	// 同帧触发，顺序不定，这里显式收尾）。
	StopWindupFollow();

	AMFCharacterBase* Char = GetMFCharacter();
	if (!Char || !ChargeData)
	{
		EndDash();
		return;
	}

	// DashDirection 已由「激活瞬间初始瞄准」或「蓄力跟随」确定，此处不再重算，
	// 保留剩余前摇作为承诺式预警（committed telegraph）。

	// 冲刺动画。
	if (ChargeData->AttackAnim)
	{
		if (UPaperZDAnimationComponent* AnimComp = Char->FindComponentByClass<UPaperZDAnimationComponent>())
		{
			if (UPaperZDAnimInstance* AnimInst = AnimComp->GetAnimInstance())
			{
				AnimInst->PlayAnimationOverride(ChargeData->AttackAnim);
			}
		}
	}

	TakeOverMovement();

	// 记录起步瞬间已重叠的 Pawn——这些目标不触发 bStopOnHit 撞停（仍会吃伤害），
	// 避免贴脸发起冲撞时第一帧立刻撞停卡在原地。
	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> StartOverlaps;
		const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 }; // ECC_Pawn
		const TArray<AActor*> Ignore = { Char };
		UKismetSystemLibrary::SphereOverlapActors(
			World, Char->GetActorLocation(), ChargeData->ChargeRadius, PawnType, nullptr, Ignore, StartOverlaps);
		for (AActor* A : StartOverlaps)
		{
			InitialOverlapActors.Add(A);
		}
	}

	MF_LOG(LogMFAbility, TEXT("[GA_Charge] %s dash begin. Dir=(%.2f,%.2f) InitialOverlaps=%d"),
		*GetNameSafe(Char), DashDirection.X, DashDirection.Y, InitialOverlapActors.Num());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DashTimer, this, &UGA_Charge::DashTick, DashTickInterval, true);
	}
}

void UGA_Charge::DashTick()
{
	AMFCharacterBase* Char = GetMFCharacter();
	UWorld* World = GetWorld();
	if (!Char || !World || !ChargeData)
	{
		EndDash();
		return;
	}

	const bool bDebug = CVarChargeDebug.GetValueOnGameThread() != 0;

	// 冲刺跟随：把冲刺方向朝当前目标转向，本帧至多 MaxDashTurnRate * dt 度。
	if (ChargeData->bFollowDuringDash)
	{
		SteerDashDirectionToward(ComputeAimDirection(), ChargeData->MaxDashTurnRate * DashTickInterval);
	}

	const FVector CurPos  = Char->GetActorLocation();
	const float   StepLen = ChargeData->ChargeSpeed * DashTickInterval;
	FVector       NewPos  = CurPos + DashDirection * StepLen;

	// 1. 撞墙检测：球形 Sweep 仅打世界几何（静态/动态），命中则停在该处。
	bool bWallBlocked = false;
	{
		FHitResult Hit;
		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MFChargeWall), false, Char);

		if (World->SweepSingleByObjectType(
				Hit, CurPos, NewPos, FQuat::Identity, ObjParams,
				FCollisionShape::MakeSphere(ChargeData->ChargeRadius), QueryParams))
		{
			NewPos = Hit.Location;
			bWallBlocked = true;
		}
	}

	// 2. 位移（不走 capsule sweep，避免被 Pawn 挡住；撞墙已在上一步处理）。
	Char->SetActorLocation(NewPos, false);
	DistanceTraveled += (NewPos - CurPos).Size();

	if (bDebug)
	{
		DrawDebugSphere(World, NewPos, ChargeData->ChargeRadius, 12, FColor::Cyan, false, 0.5f, 0, 1.5f);
	}

	// 3. 命中检测：球形 Overlap 取 Pawn，逐目标过滤，每个目标一次冲撞只结算一次。
	TArray<AActor*> Overlaps;
	const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 }; // ECC_Pawn
	const TArray<AActor*> Ignore = { Char };
	UKismetSystemLibrary::SphereOverlapActors(
		World, NewPos, ChargeData->ChargeRadius, PawnType, nullptr, Ignore, Overlaps);

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	bool bHitStop = false;

	for (AActor* Cand : Overlaps)
	{
		if (!UMFCombatStatics::PassesTargetFilter(SourceASC, Cand, ChargeData->TargetFilter))
		{
			if (bDebug)
			{
				DrawDebugSphere(World, Cand->GetActorLocation(), 40.f, 8, FColor::Orange, false, 0.5f, 0, 1.f);
			}
			continue;
		}

		bool bAlreadyHit = false;
		HitTargets.Add(Cand, &bAlreadyHit);
		if (bAlreadyHit)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cand);
		UMFCombatStatics::ApplyDamage(SourceASC, TargetASC, ChargeData->DamageGE, ChargeData->DamageMultiplier);
		UMFCombatStatics::ApplyOnHitEffects(SourceASC, TargetASC, ChargeData->OnHitEffects, GetAbilityLevel());

		if (bDebug)
		{
			DrawDebugSphere(World, Cand->GetActorLocation(), 40.f, 8, FColor::Red, false, 0.5f, 0, 3.f);
			DrawDebugString(World, Cand->GetActorLocation() + FVector(0.f, 0.f, 80.f),
				FString::Printf(TEXT("CHARGE HIT: %s"), *GetNameSafe(Cand)), nullptr, FColor::Red, 0.5f);
		}

		// 起步瞬间就重叠的目标不触发撞停（仍已吃伤害）；只有冲刺途中新撞上的才停。
		if (ChargeData->bStopOnHit && !InitialOverlapActors.Contains(Cand))
		{
			bHitStop = true;
		}
	}

	// 4. 终止条件：撞墙 / 命中即停 / 达到最远距离。
	if (bWallBlocked || bHitStop || DistanceTraveled >= ChargeData->MaxDistance)
	{
		EndDash();
	}
}

void UGA_Charge::EndDash()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTimer);
	}

	RestoreMovement();

	// 可选：冲刺终点生成持续区域（扬尘 / 减速带等）。
	if (ChargeData && ChargeData->ImpactAreaData)
	{
		if (AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			UMFCombatStatics::SpawnAreaEffect(Avatar, ChargeData->ImpactAreaData, Avatar->GetActorLocation());
		}
	}

	MF_LOG(LogMFAbility, TEXT("[GA_Charge] %s dash end. Traveled=%.0f Hits=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), DistanceTraveled, HitTargets.Num());

	// 后摇后结束技能。
	if (UWorld* World = GetWorld())
	{
		const float Recovery = ChargeData ? FMath::Max(ChargeData->RecoverySeconds, 0.01f) : 0.01f;
		World->GetTimerManager().SetTimer(RecoveryTimer, this, &UGA_Charge::FinishCharge, Recovery, false);
	}
	else
	{
		FinishCharge();
	}
}

void UGA_Charge::FinishCharge()
{
	EndAbility(CachedHandle, CurrentActorInfo, CachedActivationInfo, true, false);
}

// ============================================================================
// Movement takeover
// ============================================================================

void UGA_Charge::TakeOverMovement()
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

void UGA_Charge::RestoreMovement()
{
	AMFCharacterBase* Char = GetMFCharacter();
	if (!Char) return;

	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
	if (!CMC || CMC->MovementMode != MOVE_None) return;

	// 仍处于眩晕 / 死亡时不抢着恢复行走——由 OnStunnedTagChanged / 死亡流程各自负责，
	// 否则会在眩晕中途把移动放开。镜像 AMFCharacterBase::OnStunnedTagChanged 的判定。
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bBlocked = ASC &&
		(ASC->HasMatchingGameplayTag(MFGameplayTags::State_Stunned) ||
		 ASC->HasMatchingGameplayTag(MFGameplayTags::State_Dead));
	if (!bBlocked)
	{
		CMC->SetMovementMode(MOVE_Walking);
	}
}

void UGA_Charge::ClearAllTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(WindupTimer);
		TM.ClearTimer(WindupFollowTimer);
		TM.ClearTimer(FollowStopTimer);
		TM.ClearTimer(DashTimer);
		TM.ClearTimer(RecoveryTimer);
	}
}

// ============================================================================
// Helpers
// ============================================================================

void UGA_Charge::WindupFollowTick()
{
	// 蓄力跟随：持续把冲刺方向更新为朝向当前目标。
	DashDirection = ComputeAimDirection();
}

void UGA_Charge::StopWindupFollow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WindupFollowTimer);
	}
}

FVector UGA_Charge::ComputeAimDirection() const
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

void UGA_Charge::SteerDashDirectionToward(const FVector& Desired, float MaxStepDeg)
{
	if (Desired.IsNearlyZero() || MaxStepDeg <= 0.f)
	{
		return;
	}

	const FVector Cur = DashDirection.GetSafeNormal();
	if (Cur.IsNearlyZero())
	{
		DashDirection = Desired;
		return;
	}

	const float Dot      = FMath::Clamp(FVector::DotProduct(Cur, Desired), -1.f, 1.f);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// 目标方向已在本帧可转范围内 → 直接对齐。
	if (AngleDeg <= MaxStepDeg)
	{
		DashDirection = Desired;
		return;
	}

	// 否则朝目标方向转 MaxStepDeg 度。转向符号由叉积 Z 分量决定（绕 +Z 逆时针为正）。
	const float CrossZ = Cur.X * Desired.Y - Cur.Y * Desired.X;
	const float Sign   = (CrossZ >= 0.f) ? 1.f : -1.f;
	DashDirection = Cur.RotateAngleAxis(MaxStepDeg * Sign, FVector::UpVector).GetSafeNormal();
}

AMFAICharacter* UGA_Charge::GetAICharacter() const
{
	return Cast<AMFAICharacter>(GetAvatarActorFromActorInfo());
}

AActor* UGA_Charge::GetCurrentTarget() const
{
	AMFAICharacter* AI = GetAICharacter();
	if (!AI) return nullptr;

	UMFThreatComponent* ThreatComp = AI->FindComponentByClass<UMFThreatComponent>();
	return ThreatComp ? ThreatComp->GetCurrentTarget() : nullptr;
}

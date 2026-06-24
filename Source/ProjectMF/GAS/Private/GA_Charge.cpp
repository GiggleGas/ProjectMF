// Copyright ProjectMF. All Rights Reserved.

#include "GA_Charge.h"

#include "MFChargeData.h"
#include "MFCombatStatics.h"
#include "MFGameplayTags.h"
#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Construction / hooks
// ============================================================================

UGA_Charge::UGA_Charge()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pet_Move_Charge));
}

UMFMoveAbilityData* UGA_Charge::GetMoveData() const
{
	return ChargeData;
}

FGameplayTag UGA_Charge::GetActiveStateTag() const
{
	return MFGameplayTags::State_Charging;
}

// ============================================================================
// Dash
// ============================================================================

void UGA_Charge::BeginMovement()
{
	AMFCharacterBase* Char = GetMFCharacter();
	if (!Char || !ChargeData)
	{
		StartRecovery();
		return;
	}

	DistanceTraveled = 0.f;
	InitialOverlapActors.Reset();

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

		MF_LOG(LogMFAbility, TEXT("[GA_Charge] %s dash begin. Dir=(%.2f,%.2f) InitialOverlaps=%d"),
			*GetNameSafe(Char), AimDirection.X, AimDirection.Y, InitialOverlapActors.Num());

		World->GetTimerManager().SetTimer(MotionTimer, this, &UGA_Charge::DashTick, MFMove::TickInterval, true);
	}
}

void UGA_Charge::DashTick()
{
	AMFCharacterBase* Char = GetMFCharacter();
	UWorld* World = GetWorld();
	if (!Char || !World || !ChargeData)
	{
		StartRecovery();
		return;
	}

	const bool bDebug = IsMoveDebugEnabled();

	// 冲刺跟随：把冲刺方向朝当前目标转向，本帧至多 MaxDashTurnRate * dt 度。
	if (ChargeData->bFollowDuringDash)
	{
		SteerAimDirectionToward(ComputeAimDirection(), ChargeData->MaxDashTurnRate * MFMove::TickInterval);
	}

	const FVector CurPos  = Char->GetActorLocation();
	const float   StepLen = ChargeData->ChargeSpeed * MFMove::TickInterval;
	FVector       NewPos  = CurPos + AimDirection * StepLen;

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

		const bool bFreshHit = ApplyHitToTarget(Cand);
		if (!bFreshHit)
		{
			continue;
		}

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
		MF_LOG(LogMFAbility, TEXT("[GA_Charge] %s dash end. Traveled=%.0f Hits=%d Wall=%d"),
			*GetNameSafe(Char), DistanceTraveled, HitTargets.Num(), bWallBlocked ? 1 : 0);

		SpawnImpactArea(Char->GetActorLocation());
		StartRecovery();
	}
}

void UGA_Charge::SteerAimDirectionToward(const FVector& Desired, float MaxStepDeg)
{
	if (Desired.IsNearlyZero() || MaxStepDeg <= 0.f)
	{
		return;
	}

	const FVector Cur = AimDirection.GetSafeNormal();
	if (Cur.IsNearlyZero())
	{
		AimDirection = Desired;
		return;
	}

	const float Dot      = FMath::Clamp(FVector::DotProduct(Cur, Desired), -1.f, 1.f);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// 目标方向已在本帧可转范围内 → 直接对齐。
	if (AngleDeg <= MaxStepDeg)
	{
		AimDirection = Desired;
		return;
	}

	// 否则朝目标方向转 MaxStepDeg 度。转向符号由叉积 Z 分量决定（绕 +Z 逆时针为正）。
	const float CrossZ = Cur.X * Desired.Y - Cur.Y * Desired.X;
	const float Sign   = (CrossZ >= 0.f) ? 1.f : -1.f;
	AimDirection = Cur.RotateAngleAxis(MaxStepDeg * Sign, FVector::UpVector).GetSafeNormal();
}

// ============================================================================
// Telegraph（冲撞走廊矩形）
// ============================================================================

bool UGA_Charge::BuildTelegraphRequest(FMFTelegraphRequest& OutRequest) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ChargeData || !Avatar)
	{
		return false;
	}

	const FVector Dir = AimDirection.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return false;
	}

	const float Length = ChargeData->MaxDistance;       // 走廊长 = 最大冲刺距离
	const float Width  = ChargeData->ChargeRadius * 2.f; // 走廊宽 = 命中直径

	OutRequest.Shape    = EMFTelegraphShape::Rect;
	// 矩形中心 = 起点沿朝向前移半个长度（从脚下铺到最远点）。
	OutRequest.Location = Avatar->GetActorLocation() + Dir * (Length * 0.5f);
	OutRequest.Rotation = Dir.Rotation(); // 仅取 Yaw
	OutRequest.BoxSize  = FVector2D(Length, Width); // (长, 宽)
	OutRequest.Color    = FLinearColor(1.f, 0.15f, 0.1f, 0.5f);
	return true;
}

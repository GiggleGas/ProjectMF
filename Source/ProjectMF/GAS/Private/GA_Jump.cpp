// Copyright ProjectMF. All Rights Reserved.

#include "GA_Jump.h"

#include "MFJumpData.h"
#include "MFCombatStatics.h"
#include "MFTelegraphSubsystem.h"
#include "MFGameplayTags.h"
#include "MFCharacterBase.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Construction / hooks
// ============================================================================

UGA_Jump::UGA_Jump()
{
	SetAssetTags(FGameplayTagContainer(MFGameplayTags::Ability_Pet_Move_Jump));
}

UMFMoveAbilityData* UGA_Jump::GetMoveData() const
{
	return JumpData;
}

FGameplayTag UGA_Jump::GetActiveStateTag() const
{
	return MFGameplayTags::State_Jumping;
}

// ============================================================================
// Leap
// ============================================================================

void UGA_Jump::BeginMovement()
{
	AMFCharacterBase* Char = GetMFCharacter();
	if (!Char || !JumpData)
	{
		StartRecovery();
		return;
	}

	StartPos = Char->GetActorLocation();
	Elapsed  = 0.f;

	// 落点 = 锁定瞄准算出（已按 MaxJumpDistance 夹紧）；与前摇预警共用同一计算。
	LandPos = ComputeLandPos();

	FVector Flat = LandPos - StartPos;
	Flat.Z = 0.f;
	const float Dist = Flat.Size();

	// 弧高按 实际距离 / 最大距离 比例缩放，封顶 MaxJumpHeight——短跳低弧、满距离才到峰高，
	// 水平(MaxJumpDistance)与垂直(MaxJumpHeight)都不会冲出屏幕。
	const float Ratio = (JumpData->MaxJumpDistance > 0.f)
		? FMath::Clamp(Dist / JumpData->MaxJumpDistance, 0.f, 1.f) : 0.f;
	// 弧高按距离缩放，但不低于 MinJumpHeight——原地跳(距离≈0)也起跳到最小高度，避免"只播动画不动"。
	ArcHeight = FMath::Max(JumpData->MinJumpHeight, JumpData->MaxJumpHeight * Ratio);

	MF_LOG(LogMFAbility, TEXT("[GA_Jump] %s leap begin. Land=(%.0f,%.0f,%.0f) Dist=%.0f Dur=%.2fs ArcH=%.0f"),
		*GetNameSafe(Char), LandPos.X, LandPos.Y, LandPos.Z, Dist, JumpData->JumpDuration, ArcHeight);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MotionTimer, this, &UGA_Jump::JumpTick, MFMove::TickInterval, true);
	}
}

void UGA_Jump::JumpTick()
{
	AMFCharacterBase* Char = GetMFCharacter();
	UWorld* World = GetWorld();
	if (!Char || !World || !JumpData)
	{
		DoLanding();
		return;
	}

	Elapsed += MFMove::TickInterval;
	const float Dur = FMath::Max(JumpData->JumpDuration, 0.01f);
	const float t   = FMath::Clamp(Elapsed / Dur, 0.f, 1.f);

	// 抛物线：水平线性插值 + 垂直 4·ArcHeight·t·(1-t)（t=0.5 峰值 ArcHeight）。
	FVector Pos = FMath::Lerp(StartPos, LandPos, t);
	Pos.Z += 4.f * ArcHeight * t * (1.f - t);
	Char->SetActorLocation(Pos, false);

	if (IsMoveDebugEnabled())
	{
		DrawDebugSphere(World, Pos, 30.f, 8, FColor::Cyan, false, 0.5f, 0, 1.5f);
	}

	if (t >= 1.f)
	{
		DoLanding();
	}
}

void UGA_Jump::DoLanding()
{
	AMFCharacterBase* Char = GetMFCharacter();
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(MotionTimer);
	}

	if (Char && World && JumpData)
	{
		// 贴到落点，再做落地 AOE。
		Char->SetActorLocation(LandPos, false);

		TArray<AActor*> Overlaps;
		const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 }; // ECC_Pawn
		const TArray<AActor*> Ignore = { Char };
		UKismetSystemLibrary::SphereOverlapActors(
			World, LandPos, JumpData->ImpactRadius, PawnType, nullptr, Ignore, Overlaps);

		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		const bool bDebug = IsMoveDebugEnabled();

		for (AActor* Cand : Overlaps)
		{
			if (!UMFCombatStatics::PassesTargetFilter(SourceASC, Cand, JumpData->TargetFilter))
			{
				if (bDebug)
				{
					DrawDebugSphere(World, Cand->GetActorLocation(), 40.f, 8, FColor::Orange, false, 1.f, 0, 1.f);
				}
				continue;
			}

			if (ApplyHitToTarget(Cand) && bDebug)
			{
				DrawDebugSphere(World, Cand->GetActorLocation(), 40.f, 8, FColor::Red, false, 1.f, 0, 3.f);
				DrawDebugString(World, Cand->GetActorLocation() + FVector(0.f, 0.f, 80.f),
					FString::Printf(TEXT("JUMP HIT: %s"), *GetNameSafe(Cand)), nullptr, FColor::Red, 1.f);
			}
		}

		if (bDebug)
		{
			DrawDebugCircle(World, LandPos, JumpData->ImpactRadius, 24, FColor::Red, false, 1.f, 0, 4.f,
				FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		}

		SpawnImpactArea(LandPos);
	}

	MF_LOG(LogMFAbility, TEXT("[GA_Jump] %s landed. Hits=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), HitTargets.Num());

	StartRecovery();
}

// ============================================================================
// Telegraph（落点圆 = 落地 AOE 范围）
// ============================================================================

FVector UGA_Jump::ComputeLandPos() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !JumpData)
	{
		return FVector::ZeroVector;
	}

	const FVector Origin = Avatar->GetActorLocation();

	// 落点 = 锁定的目标位置（直接选目标点）；无目标则用瞄准方向定距到最大距离。
	FVector Land = bAimHasTarget ? AimTargetLocation : (Origin + AimDirection * JumpData->MaxJumpDistance);

	// 水平距离夹紧到 MaxJumpDistance（Z 保留落点高度）。
	FVector Flat = Land - Origin;
	Flat.Z = 0.f;
	const float Dist = Flat.Size();
	if (Dist > JumpData->MaxJumpDistance)
	{
		const FVector Dir = Flat / Dist;
		Land.X = Origin.X + Dir.X * JumpData->MaxJumpDistance;
		Land.Y = Origin.Y + Dir.Y * JumpData->MaxJumpDistance;
	}
	return Land;
}

bool UGA_Jump::BuildTelegraphRequest(FMFTelegraphRequest& OutRequest) const
{
	if (!JumpData)
	{
		return false;
	}
	OutRequest.Shape    = EMFTelegraphShape::Circle;
	OutRequest.Location = ComputeLandPos();
	OutRequest.Radius   = JumpData->ImpactRadius;
	OutRequest.Color    = FLinearColor(1.f, 0.15f, 0.1f, 0.5f); // 半透明红：落地 AOE 预警
	return true;
}

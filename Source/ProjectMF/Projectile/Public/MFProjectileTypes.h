// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MFAttackTypes.h"           // EAttackTargetFilter

class UPaperFlipbook;
class UGameplayEffect;

// ============================================================================
// Enums
// ============================================================================

/** Why a projectile stopped simulating. Passed to the GA's OnResolved callback. */
enum class EMFProjectileResolveReason : uint8
{
	HitTarget,   // 命中了通过 PassesTargetFilter 的目标
	HitGround,   // 抛物线下坠命中地面 / 到达指定落点（玩家投掷生成区域用）
	MaxRange,    // 飞行距离达到 MaxRange，未命中目标
	Cancelled,   // GA 主动取消（EndAbility bWasCancelled=true）
};

// ============================================================================
// Result
// ============================================================================

/** Payload delivered to the GA callback when a projectile resolves. */
struct FMFProjectileResult
{
	EMFProjectileResolveReason      Reason       = EMFProjectileResolveReason::Cancelled;
	TWeakObjectPtr<AActor>          HitActor;           // 仅 HitTarget 时有效
	FVector                         FinalPosition = FVector::ZeroVector;
};

// ============================================================================
// Delegate
// ============================================================================

/** Single-cast, non-dynamic delegate fired when a projectile resolves. */
DECLARE_DELEGATE_OneParam(FOnProjectileResolved, const FMFProjectileResult&)

// ============================================================================
// Handle
// ============================================================================

/**
 * Opaque handle returned by UMFProjectileSubsystem::Launch.
 * GA holds this to cancel the projectile early (EndAbility bWasCancelled).
 * UID=0 means invalid / not yet assigned.
 */
struct FMFProjectileHandle
{
	uint32 UID = 0;

	bool IsValid()    const { return UID != 0; }
	void Invalidate()       { UID = 0; }

	bool operator==(const FMFProjectileHandle& Other) const { return UID == Other.UID; }
	bool operator!=(const FMFProjectileHandle& Other) const { return UID != Other.UID; }
};

// ============================================================================
// Launch Params  (GA → Subsystem)
// ============================================================================

/**
 * All parameters needed to start simulating one projectile.
 * The GA fills this and passes it to UMFProjectileSubsystem::Launch.
 *
 * 弹道用**速度矢量 + 重力**表达（统一直线与抛物线）：
 *   直线（AI 投掷/落石/弹幕）：Velocity = Dir*Speed, GravityZ = 0
 *   抛物线（玩家投掷消耗品）  ：Velocity = 反算初速度, GravityZ > 0
 */
struct FMFProjectileLaunchParams
{
	FVector                         Origin          = FVector::ZeroVector;
	FVector                         Velocity        = FVector::ForwardVector; // cm/s，含方向与大小
	float                           GravityZ        = 0.f;      // cm/s²，0=直线，>0=抛物线下坠
	float                           MaxRange        = 1500.f;   // cm（按累计路程判定）
	float                           CollisionRadius = 15.f;     // sweep sphere radius

	UPaperFlipbook*                 Flipbook        = nullptr;  // 2D 外观（Flipbook 池渲染；单帧=静态）
	float                           VisualScale     = 1.f;      // 投射物视觉缩放
	TWeakObjectPtr<AActor>          Instigator;                 // ignored in filter + sweep
	TSubclassOf<UGameplayEffect>    DamageGE;
	float                           DamageMultiplier = 1.f;
	EAttackTargetFilter             TargetFilter     = EAttackTargetFilter::EnemyOnly;

	FOnProjectileResolved           OnResolved;     // bound by GA before passing to Launch
};

// ============================================================================
// Instance  (Subsystem internal state, slot-based)
// ============================================================================

/**
 * Live simulation state for one projectile inside UMFProjectileSubsystem.
 * Stored in a flat TArray; bActive=false means the slot is free.
 *
 * GC note: Flipbook and DamageGE are raw/TSubclassOf pointers without UPROPERTY.
 * They are safe during normal gameplay because both originate from DataAssets
 * that are strongly referenced by the GA (which is on a live ASC).
 */
struct FMFProjectileInstance
{
	uint32                          UID              = 0;
	bool                            bActive          = false;

	FVector                         CurrentPos       = FVector::ZeroVector;
	FVector                         Velocity         = FVector::ZeroVector;  // cm/s，每帧受 GravityZ 影响
	float                           GravityZ         = 0.f;
	float                           MaxRange         = 0.f;
	float                           DistanceTraveled = 0.f;
	float                           CollisionRadius  = 15.f;

	UPaperFlipbook*                 Flipbook         = nullptr;
	int32                           SlotIndex        = -1;      // Flipbook 池 slot，-1 = 无

	TWeakObjectPtr<AActor>          Instigator;
	TSubclassOf<UGameplayEffect>    DamageGE;
	float                           DamageMultiplier = 1.f;
	EAttackTargetFilter             TargetFilter     = EAttackTargetFilter::EnemyOnly;

	FOnProjectileResolved           OnResolved;

	void InitFromParams(const FMFProjectileLaunchParams& Params, uint32 InUID)
	{
		UID              = InUID;
		bActive          = true;
		CurrentPos       = Params.Origin;
		Velocity         = Params.Velocity;
		GravityZ         = Params.GravityZ;
		MaxRange         = Params.MaxRange;
		DistanceTraveled = 0.f;
		CollisionRadius  = Params.CollisionRadius;
		Flipbook         = Params.Flipbook;
		SlotIndex        = -1;
		Instigator       = Params.Instigator;
		DamageGE         = Params.DamageGE;
		DamageMultiplier = Params.DamageMultiplier;
		TargetFilter     = Params.TargetFilter;
		OnResolved       = Params.OnResolved;
	}

	void Reset()
	{
		UID              = 0;
		bActive          = false;
		CurrentPos       = FVector::ZeroVector;
		Velocity         = FVector::ZeroVector;
		GravityZ         = 0.f;
		MaxRange         = 0.f;
		DistanceTraveled = 0.f;
		CollisionRadius  = 15.f;
		Flipbook         = nullptr;
		SlotIndex        = -1;
		Instigator       = nullptr;
		DamageGE         = nullptr;
		DamageMultiplier = 1.f;
		OnResolved.Unbind();
	}
};

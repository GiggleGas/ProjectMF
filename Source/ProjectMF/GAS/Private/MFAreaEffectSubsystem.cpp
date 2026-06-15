// Copyright ProjectMF. All Rights Reserved.

#include "MFAreaEffectSubsystem.h"
#include "MFAreaEffectData.h"
#include "MFCombatStatics.h"
#include "MFGameplayTags.h"
#include "MFFactionStatics.h"
#include "MFLog.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Debug
// ============================================================================

static TAutoConsoleVariable<int32> CVarAreaDebug(
	TEXT("mf.debug.area"),
	0,
	TEXT("0=off  1=draw area-effect spheres and log per-tick hit counts."),
	ECVF_Cheat);

namespace
{
	/** 区域目标过滤：跳过死亡 + 阵营判定（同 GA FilterTarget）。 */
	bool AreaPassesFilter(UAbilitySystemComponent* Source, UAbilitySystemComponent* Cand, EAttackTargetFilter Filter)
	{
		if (!Cand) return false;
		if (Cand->HasMatchingGameplayTag(MFGameplayTags::State_Dead)) return false;
		if (Filter == EAttackTargetFilter::All) return true;
		if (!Source) return false;
		const bool bSame = UMFFactionStatics::AreSameTeam(Source, Cand);
		return (Filter == EAttackTargetFilter::EnemyOnly) ? !bSame : bSame;
	}
}

// ============================================================================
// Register / Cancel
// ============================================================================

FMFAreaHandle UMFAreaEffectSubsystem::RegisterArea(AActor* Instigator, const UMFAreaEffectData* Data, const FVector& Location)
{
	if (!Data)
	{
		return FMFAreaHandle();
	}

	int32 Slot;
	if (FreeSlots.Num() > 0)
	{
		Slot = FreeSlots.Pop(EAllowShrinking::No);
	}
	else
	{
		Slot = Instances.AddDefaulted();
	}

	FAreaInstance& Inst   = Instances[Slot];
	Inst.UID              = NextUID++;
	Inst.bActive          = true;
	Inst.Location         = Location;
	Inst.Radius           = Data->Radius;
	Inst.RemainingLife    = Data->Duration;
	Inst.TickInterval     = FMath::Max(Data->TickInterval, 0.01f);
	Inst.TickAccumulator  = Inst.TickInterval;   // 注册后第一帧立即施加一次
	Inst.TargetFilter     = Data->TargetFilter;
	Inst.DamageGE         = Data->DamageGE;
	Inst.DamageMultiplier = Data->DamageMultiplier;
	Inst.Effects          = Data->Effects;
	Inst.Instigator       = Instigator;
	Inst.VisualActor      = nullptr;

	// 表现：随场生成展示 Actor（可空），按需等比缩放以匹配半径。
	if (Data->VisualActorClass)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Visual = World->SpawnActor<AActor>(
				Data->VisualActorClass, FTransform(FRotator::ZeroRotator, Location), SpawnParams);
			if (Visual)
			{
				if (Data->VisualBaseRadius > 0.f)
				{
					Visual->SetActorScale3D(FVector(Data->Radius / Data->VisualBaseRadius));
				}
				Inst.VisualActor = Visual;
			}
		}
	}

	++ActiveCount;

	FMFAreaHandle Handle;
	Handle.UID = Inst.UID;
	return Handle;
}

void UMFAreaEffectSubsystem::Cancel(FMFAreaHandle Handle)
{
	if (!Handle.IsValid()) return;

	const int32 Slot = FindSlotByUID(Handle.UID);
	if (Slot == INDEX_NONE || !Instances[Slot].bActive) return;

	FAreaInstance& Inst = Instances[Slot];
	Inst.bActive = false;
	Inst.Effects.Reset();
	Inst.Instigator = nullptr;
	if (AActor* Visual = Inst.VisualActor.Get()) { Visual->Destroy(); }
	Inst.VisualActor = nullptr;
	FreeSlots.Add(Slot);
	--ActiveCount;
}

int32 UMFAreaEffectSubsystem::FindSlotByUID(uint32 UID) const
{
	for (int32 i = 0; i < Instances.Num(); ++i)
	{
		if (Instances[i].bActive && Instances[i].UID == UID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// ============================================================================
// Tick
// ============================================================================

void UMFAreaEffectSubsystem::Tick(float DeltaTime)
{
	// 用 index 访问（不持有 ref）：施加 GE 可能触发死亡 → 未来 re-enter RegisterArea 重分配数组也安全。
	for (int32 i = 0; i < Instances.Num(); ++i)
	{
		if (!Instances[i].bActive) continue;

		Instances[i].RemainingLife   -= DeltaTime;
		Instances[i].TickAccumulator += DeltaTime;

		int32 Guard = 0;
		while (Instances[i].bActive && Instances[i].TickAccumulator >= Instances[i].TickInterval && Guard++ < 4)
		{
			Instances[i].TickAccumulator -= Instances[i].TickInterval;
			ApplyAreaTick(Instances[i]);
		}

		if (Instances[i].bActive && Instances[i].RemainingLife <= 0.f)
		{
			Instances[i].bActive    = false;
			Instances[i].Effects.Reset();
			Instances[i].Instigator = nullptr;
			if (AActor* Visual = Instances[i].VisualActor.Get()) { Visual->Destroy(); }
			Instances[i].VisualActor = nullptr;
			FreeSlots.Add(i);
			--ActiveCount;
		}
	}
}

void UMFAreaEffectSubsystem::ApplyAreaTick(const FAreaInstance& Inst)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 先快照需要的字段：后续施加 GE 可能触发死亡，将来死亡回调若 re-enter RegisterArea
	// 会重分配 Instances，使本 Inst 引用悬挂——快照后只用局部变量，规避此风险。
	const FVector Loc                          = Inst.Location;
	const float   R                            = Inst.Radius;
	const EAttackTargetFilter Filter           = Inst.TargetFilter;
	const TSubclassOf<UGameplayEffect> DmgGE   = Inst.DamageGE;
	const float   DmgMult                      = Inst.DamageMultiplier;
	const TArray<FMFOnHitEffect> Effects       = Inst.Effects;
	AActor* Instig                             = Inst.Instigator.Get();

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instig);

	const TArray<TEnumAsByte<EObjectTypeQuery>> PawnType = { ObjectTypeQuery3 };   // ECC_Pawn
	TArray<AActor*> Ignore;
	if (Instig) { Ignore.Add(Instig); }

	TArray<AActor*> Hits;
	UKismetSystemLibrary::SphereOverlapActors(World, Loc, R, PawnType, nullptr, Ignore, Hits);

	const bool bDebug = CVarAreaDebug.GetValueOnGameThread() != 0;
	if (bDebug)
	{
		DrawDebugSphere(World, Loc, R, 16, FColor::Magenta, false, Inst.TickInterval, 0, 1.5f);
	}

	int32 HitCount = 0;
	for (AActor* Tgt : Hits)
	{
		UAbilitySystemComponent* TgtASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Tgt);
		if (!AreaPassesFilter(SourceASC, TgtASC, Filter)) continue;

		if (DmgMult > 0.f && DmgGE)
		{
			UMFCombatStatics::ApplyDamage(SourceASC, TgtASC, DmgGE, DmgMult);
		}
		if (Effects.Num() > 0)
		{
			UMFCombatStatics::ApplyOnHitEffects(SourceASC, TgtASC, Effects);
		}
		++HitCount;
	}

	if (bDebug)
	{
		MF_LOG(LogMFAbility, TEXT("[Area] tick at (%.0f,%.0f,%.0f) r=%.0f hit=%d"),
			Loc.X, Loc.Y, Loc.Z, R, HitCount);
	}
}

// ============================================================================
// FTickableGameObject
// ============================================================================

bool UMFAreaEffectSubsystem::IsTickable() const
{
	return ActiveCount > 0;
}

TStatId UMFAreaEffectSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMFAreaEffectSubsystem, STATGROUP_Tickables);
}

// ============================================================================
// Debug command: mf.debug.spawnarea
// ============================================================================

static FAutoConsoleCommandWithWorld GMFSpawnAreaCmd(
	TEXT("mf.debug.spawnarea"),
	TEXT("Spawn a test area-effect at the local player pawn (Slow + TargetFilter=All). Turn on mf.debug.area 1 to visualize."),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World)
	{
		if (!World) return;
		APlayerController* PC = World->GetFirstPlayerController();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		UMFAreaEffectSubsystem* Sub = World->GetSubsystem<UMFAreaEffectSubsystem>();
		if (!Pawn || !Sub) return;

		UMFAreaEffectData* Data = NewObject<UMFAreaEffectData>(GetTransientPackage());
		Data->Radius       = 400.f;
		Data->Duration     = 5.f;
		Data->TickInterval = 0.5f;
		Data->TargetFilter = EAttackTargetFilter::All;
		// 命令拿不到 BP GE 资产，故不配伤害（DamageMultiplier=0）；
		// 靠 mf.debug.area 看 overlap/tick；若映射表配了 Slow，范围内目标也会被减速。
		FMFOnHitEffect Eff;
		Eff.Kind = EMFOnHitEffectKind::Slow;
		Eff.Chance = 1.f;
		Eff.Duration = 1.f;
		Eff.Magnitude = 0.5f;
		Data->Effects.Add(Eff);

		Sub->RegisterArea(Pawn, Data, Pawn->GetActorLocation());
		MF_LOG(LogMFAbility, TEXT("[mf.debug.spawnarea] spawned at %s (mf.debug.area 1 to visualize)"),
			*Pawn->GetActorLocation().ToString());
	}));

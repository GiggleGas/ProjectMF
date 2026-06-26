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
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Debug
// ============================================================================

static TAutoConsoleVariable<int32> CVarAreaDebug(
	TEXT("mf.debug.area"),
	0,
	TEXT("0=off  1=每帧绘制场的球体 + 文字(类型/剩余时间/半径/伤害/过滤/效果)，并按 tick 记录命中数。"),
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
		// EnemyOnly 走 AreHostile（中立不被误伤）；AllyOnly 走 AreSameTeam。
		return (Filter == EAttackTargetFilter::EnemyOnly)
			? UMFFactionStatics::AreHostile(Source, Cand)
			: UMFFactionStatics::AreSameTeam(Source, Cand);
	}

	/** 目标过滤枚举 → 字符串（调试文字用）。 */
	FString FilterToString(EAttackTargetFilter Filter)
	{
		if (const UEnum* E = StaticEnum<EAttackTargetFilter>())
		{
			return E->GetNameStringByValue(static_cast<int64>(Filter));
		}
		return TEXT("?");
	}

	/** 效果列表 → "Slow,Stun" 形式（调试文字用）。 */
	FString EffectsToString(const TArray<FMFOnHitEffect>& Effects)
	{
		if (Effects.Num() == 0) return TEXT("无");
		const UEnum* KindEnum = StaticEnum<EMFOnHitEffectKind>();
		FString Out;
		for (int32 i = 0; i < Effects.Num(); ++i)
		{
			if (i > 0) Out += TEXT(",");
			Out += KindEnum ? KindEnum->GetNameStringByValue(static_cast<int64>(Effects[i].Kind)) : TEXT("?");
		}
		return Out;
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
	Inst.DebugTypeName    = Data->GetFName();

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
	UWorld*    World  = GetWorld();
	const bool bDebug = CVarAreaDebug.GetValueOnGameThread() != 0;

	// 用 index 访问（不持有 ref）：施加 GE 可能触发死亡 → 未来 re-enter RegisterArea 重分配数组也安全。
	for (int32 i = 0; i < Instances.Num(); ++i)
	{
		if (!Instances[i].bActive) continue;

		Instances[i].RemainingLife   -= DeltaTime;
		Instances[i].TickAccumulator += DeltaTime;

		// 每帧绘制：球 + 文字（类型 / 剩余时间 / 半径 / 伤害倍率 / 过滤 / 效果）。
		// 有伤害=红，纯控制=青。文字 lifetime=0 即单帧，靠逐帧重绘维持。
		if (bDebug && World)
		{
			const FAreaInstance& D = Instances[i];
			const bool   bDeals = (D.DamageMultiplier > 0.f && D.DamageGE != nullptr);
			const FColor Color  = bDeals ? FColor::Red : FColor::Cyan;

			DrawDebugSphere(World, D.Location, D.Radius, 24, Color, false, 0.f, 0, 2.f);

			const FString Text = FString::Printf(
				TEXT("%s\n剩余 %.1fs\nr=%.0f  dmgx%.2f\nfilter=%s\neffects=%s"),
				*D.DebugTypeName.ToString(),
				FMath::Max(D.RemainingLife, 0.f),
				D.Radius, D.DamageMultiplier,
				*FilterToString(D.TargetFilter),
				*EffectsToString(D.Effects));
			DrawDebugString(World, D.Location + FVector(0.f, 0.f, 40.f), Text, nullptr, Color, 0.f, true);
		}

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

	// 注：可视化（球 + 文字）由 Tick 每帧绘制，这里只保留每次施加的命中数日志。
	const bool bDebug = CVarAreaDebug.GetValueOnGameThread() != 0;

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

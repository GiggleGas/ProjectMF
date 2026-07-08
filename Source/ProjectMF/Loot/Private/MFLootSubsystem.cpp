// Copyright ProjectMF. All Rights Reserved.

#include "MFLootSubsystem.h"
#include "MFLootTable.h"
#include "MFLootPickup.h"
#include "MFLootSettings.h"
#include "MFLog.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Roll
// ---------------------------------------------------------------------------

TArray<FMFLootResult> UMFLootSubsystem::RollTable(const UMFLootTable* Table) const
{
	TArray<FMFLootResult> Results;
	if (!Table) return Results;

	for (const FMFLootEntry& Entry : Table->Entries)
	{
		const int32 EntryItemID = Entry.Item.ItemID;
		if (EntryItemID <= 0) continue;
		if (FMath::FRand() > Entry.Chance) continue;

		const int32 Max   = FMath::Max(Entry.CountMin, Entry.CountMax);
		const int32 Count = FMath::RandRange(Entry.CountMin, Max);
		if (Count <= 0) continue;

		// 同 ItemID 合并成一个 Result（生成时也只出一个 Pickup）。
		FMFLootResult* Existing = Results.FindByPredicate(
			[EntryItemID](const FMFLootResult& R) { return R.ItemID == EntryItemID; });
		if (Existing)
		{
			Existing->Count += Count;
		}
		else
		{
			FMFLootResult& NewResult = Results.AddDefaulted_GetRef();
			NewResult.ItemID = EntryItemID;
			NewResult.Count  = Count;
		}
	}
	return Results;
}

// ---------------------------------------------------------------------------
// Spawn
// ---------------------------------------------------------------------------

void UMFLootSubsystem::SpawnLoot(const TArray<FMFLootResult>& Results, const FVector& Location)
{
	if (Results.IsEmpty()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const TSubclassOf<AMFLootPickup> PickupClass = ResolvePickupClass();
	if (!PickupClass) return;

	const UMFLootSettings* Settings = GetDefault<UMFLootSettings>();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FMFLootResult& Result : Results)
	{
		// 一个掉落物 = 一堆（带数量）；捡起时整堆进背包，无需逐个生成。
		AMFLootPickup* Pickup = World->SpawnActor<AMFLootPickup>(
			PickupClass, Location, FRotator::ZeroRotator, SpawnParams);
		if (!Pickup) continue;

		Pickup->InitLoot(Result.ItemID, Result.Count,
			ResolveLandLocation(Location, Settings->ScatterRadius));
	}
}

void UMFLootSubsystem::DropFromTable(const UMFLootTable* Table, const FVector& Location)
{
	SpawnLoot(RollTable(Table), Location);
}

// ---------------------------------------------------------------------------
// 内部
// ---------------------------------------------------------------------------

TSubclassOf<AMFLootPickup> UMFLootSubsystem::ResolvePickupClass()
{
	if (CachedPickupClass) return CachedPickupClass;

	const UMFLootSettings* Settings = GetDefault<UMFLootSettings>();
	if (!Settings->PickupClass.IsNull())
	{
		CachedPickupClass = Settings->PickupClass.LoadSynchronous();
	}

	if (!CachedPickupClass)
	{
		if (!bWarnedNoPickupClass)
		{
			bWarnedNoPickupClass = true;
			MF_LOG_WARNING(LogMFLoot,
				TEXT("MF Loot Settings 未配置 PickupClass，回退 C++ AMFLootPickup（无外观）。")
				TEXT("请创建 BP_LootPickup 并在 Project Settings → Game → MF Loot 指定。"));
		}
		CachedPickupClass = AMFLootPickup::StaticClass();
	}
	return CachedPickupClass;
}

FVector UMFLootSubsystem::ResolveLandLocation(const FVector& Origin, float ScatterRadius) const
{
	const FVector2D Offset2D = FMath::RandPointInCircle(ScatterRadius);
	FVector Land = Origin + FVector(Offset2D.X, Offset2D.Y, 0.f);

	// 向下 trace 对齐地面（掉落点可能在斜坡/高台边缘）。不中则保持原 Z。
	FHitResult Hit;
	const FVector TraceStart = Land + FVector(0, 0, 100.f);
	const FVector TraceEnd   = Land - FVector(0, 0, 1000.f);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility))
	{
		Land.Z = Hit.ImpactPoint.Z + 5.f;
	}
	return Land;
}

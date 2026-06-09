// Copyright ProjectMF. All Rights Reserved.

#include "MFInventoryComponent.h"
#include "MFItemDatabase.h"
#include "MFAIRegistry.h"
#include "MFPetConfig.h"
#include "MFPetBase.h"
#include "MFPetAIController.h"
#include "MFAttributeSetBase.h"
#include "MFFactionStatics.h"
#include "MFRadarSensingComponent.h"
#include "MFLog.h"
#include "MFCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// ============================================================
// Debug Console Command: MF.Inventory.Debug
// ============================================================

static void PrintInventoryDebug(const TArray<FString>& Args, UWorld* World)
{
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	AMFCharacter* Player  = PC ? Cast<AMFCharacter>(PC->GetPawn()) : nullptr;
	if (!Player)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, TEXT("[Inventory] No player found."));
		return;
	}

	UMFInventoryComponent* Inv = Player->GetInventoryComponent();
	if (!Inv)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, TEXT("[Inventory] No InventoryComponent on player."));
		return;
	}

	const TArray<FMFInventorySlot>& Resources = Inv->GetResourceSlots();
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
		FString::Printf(TEXT("=== Inventory [%d resource slot(s)] ==="), Resources.Num()));

	for (int32 i = Resources.Num() - 1; i >= 0; --i)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White,
			FString::Printf(TEXT("  [%d] %s"), i, *Resources[i].GetDebugString()));
	}

	const TArray<FMFPetInstance>& Pets = Inv->GetAllPets();
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
		FString::Printf(TEXT("=== Pets [%d] ==="), Pets.Num()));

	for (int32 i = Pets.Num() - 1; i >= 0; --i)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
			FString::Printf(TEXT("  [%d] %s"), i, *Pets[i].GetDebugString()));
	}
}

static FAutoConsoleCommandWithWorldAndArgs GCmdInventoryDebug(
	TEXT("MF.Inventory.Debug"),
	TEXT("打印当前玩家背包内容（资源 + 宠物）到屏幕。"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrintInventoryDebug)
);

// ============================================================
// 构造
// ============================================================

UMFInventoryComponent::UMFInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================
// 资源
// ============================================================

int32 UMFInventoryComponent::AddResource(FName ItemID, int32 Count)
{
	if (Count <= 0 || ItemID.IsNone())
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddResource: invalid args (ID='%s', Count=%d)."), *ItemID.ToString(), Count);
		return 0;
	}

	const FMFItemDef* Def = ItemDatabase ? ItemDatabase->FindItem(ItemID) : nullptr;
	if (!Def)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddResource: ItemID '%s' not found in database."), *ItemID.ToString());
		return 0;
	}

	if (Def->ItemType != EMFItemType::Resource)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddResource: '%s' is not a Resource type."), *ItemID.ToString());
		return 0;
	}

	const int32 MaxStack = Def->MaxStackSize;
	int32 Remaining = Count;

	const int32 SlotIdx = FindResourceSlotIndex(ItemID);
	if (SlotIdx != INDEX_NONE)
	{
		FMFInventorySlot& Slot = ResourceSlots[SlotIdx];
		const int32 Space = MaxStack - Slot.Count;
		const int32 Added = FMath::Min(Space, Remaining);
		Slot.Count += Added;
		Remaining  -= Added;
	}

	if (Remaining > 0)
	{
		if (MaxResourceSlots > 0 && ResourceSlots.Num() >= MaxResourceSlots)
		{
			MF_LOG_WARNING(LogMFInventory,
				TEXT("AddResource: Backpack full (%d slots). '%s' x%d lost."),
				MaxResourceSlots, *ItemID.ToString(), Remaining);
		}
		else
		{
			FMFInventorySlot NewSlot;
			NewSlot.ItemID = ItemID;
			NewSlot.Count  = FMath::Min(Remaining, MaxStack);
			ResourceSlots.Add(NewSlot);
			Remaining -= NewSlot.Count;
		}
	}

	const int32 ActualAdded = Count - Remaining;
	if (ActualAdded > 0)
	{
		MF_LOG(LogMFInventory, TEXT("AddResource: +%d '%s' (total: %d)."),
			ActualAdded, *ItemID.ToString(), GetResourceCount(ItemID));
		OnInventoryChanged.Broadcast();
	}

	return ActualAdded;
}

bool UMFInventoryComponent::RemoveResource(FName ItemID, int32 Count)
{
	if (Count <= 0 || !HasResource(ItemID, Count))
	{
		return false;
	}

	int32 ToRemove = Count;

	for (int32 i = ResourceSlots.Num() - 1; i >= 0 && ToRemove > 0; --i)
	{
		if (ResourceSlots[i].ItemID != ItemID) continue;

		const int32 Removed = FMath::Min(ResourceSlots[i].Count, ToRemove);
		ResourceSlots[i].Count -= Removed;
		ToRemove               -= Removed;

		if (ResourceSlots[i].Count == 0)
		{
			ResourceSlots.RemoveAt(i);
		}
	}

	MF_LOG(LogMFInventory, TEXT("RemoveResource: -%d '%s' (remaining: %d)."),
		Count, *ItemID.ToString(), GetResourceCount(ItemID));
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UMFInventoryComponent::GetResourceCount(FName ItemID) const
{
	int32 Total = 0;
	for (const FMFInventorySlot& Slot : ResourceSlots)
	{
		if (Slot.ItemID == ItemID) Total += Slot.Count;
	}
	return Total;
}

bool UMFInventoryComponent::HasResource(FName ItemID, int32 Count) const
{
	return GetResourceCount(ItemID) >= Count;
}

int32 UMFInventoryComponent::FindResourceSlotIndex(FName ItemID) const
{
	for (int32 i = 0; i < ResourceSlots.Num(); ++i)
	{
		if (ResourceSlots[i].ItemID == ItemID) return i;
	}
	return INDEX_NONE;
}

// ============================================================
// 宠物 — 捕获
// ============================================================

FGuid UMFInventoryComponent::RegisterCaughtPet(const FMFPetInstance& Snapshot)
{
	if (Snapshot.AIConfigID.IsNone())
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("RegisterCaughtPet: AIConfigID is None. 请在 UMFPetConfig 中设置 AIConfigID。"));
		return FGuid();
	}

	if (MaxPetSlots > 0 && PetSlots.Num() >= MaxPetSlots)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("RegisterCaughtPet: Pet roster full (%d/%d)."), PetSlots.Num(), MaxPetSlots);
		return FGuid();
	}

	FMFPetInstance NewInstance = Snapshot;   // 复制快照（含 AIConfigID + AttributeSnapshot）
	NewInstance.InstanceID     = FGuid::NewGuid();
	NewInstance.Level          = 1;
	NewInstance.Experience     = 0;
	NewInstance.bIsActive      = false;

	// 默认昵称从 AIRegistry 读取
	if (NewInstance.PetName.IsEmpty() && AIRegistry)
	{
		if (const FMFAIRegistryRow* Row = AIRegistry->FindRow<FMFAIRegistryRow>(
				Snapshot.AIConfigID, TEXT("RegisterCaughtPet")))
		{
			if (UMFPetConfig* Config = Row->Config.LoadSynchronous())
			{
				NewInstance.PetName = Config->DisplayName.ToString();
			}
		}
	}
	if (NewInstance.PetName.IsEmpty())
	{
		NewInstance.PetName = Snapshot.AIConfigID.ToString();
	}

	PetSlots.Add(NewInstance);

	MF_LOG(LogMFInventory, TEXT("RegisterCaughtPet: '%s' (%s) registered. Total: %d."),
		*NewInstance.PetName, *Snapshot.AIConfigID.ToString(), PetSlots.Num());
	OnPetRosterChanged.Broadcast();

	return NewInstance.InstanceID;
}

// ============================================================
// 宠物 — 召唤 / 召回
// ============================================================

AMFPetBase* UMFInventoryComponent::SummonPet(FGuid InstanceID, FVector Location)
{
	FMFPetInstance* InstancePtr = FindPetMutable(InstanceID);
	if (!InstancePtr)
	{
		MF_LOG_WARNING(LogMFInventory, TEXT("SummonPet: InstanceID not found."));
		return nullptr;
	}

	if (InstancePtr->bIsDead)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("SummonPet: '%s' is reviving (%.0fs left) — cannot summon."),
			*InstancePtr->PetName, InstancePtr->ReviveTimeRemaining);
		return nullptr;
	}

	// 重复召唤保护：已在场且 Actor 有效，直接返回现有，避免重复 Spawn 泄漏旧 Actor。
	if (AMFPetBase* Existing = GetActivePetActor(InstanceID))
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("SummonPet: '%s' is already summoned — returning existing actor."),
			*InstancePtr->PetName);
		return Existing;
	}
	// 状态残留清理：标记出战但 Actor 已失效（被外部销毁），重置后继续正常召唤。
	if (InstancePtr->bIsActive)
	{
		ActivePetActors.Remove(InstanceID);
		InstancePtr->bIsActive = false;
	}

	if (!AIRegistry)
	{
		MF_LOG_ERROR(LogMFInventory, TEXT("SummonPet: AIRegistry not set. 请在 BP_MFCharacter 的 InventoryComponent 中赋值 DT_AIRegistry。"));
		return nullptr;
	}

	const FMFAIRegistryRow* Row = AIRegistry->FindRow<FMFAIRegistryRow>(
		InstancePtr->AIConfigID, TEXT("SummonPet"));
	if (!Row)
	{
		MF_LOG_ERROR(LogMFInventory, TEXT("SummonPet: AIConfigID '%s' not found in AIRegistry."),
			*InstancePtr->AIConfigID.ToString());
		return nullptr;
	}

	UMFPetConfig* Config = Row->Config.LoadSynchronous();
	if (!Config || !Config->PetClass)
	{
		MF_LOG_ERROR(LogMFInventory, TEXT("SummonPet: Config or PetClass missing for '%s'."),
			*InstancePtr->AIConfigID.ToString());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMFPetBase* SpawnedPet = World->SpawnActor<AMFPetBase>(
		Config->PetClass, Location, FRotator::ZeroRotator, SpawnParams);

	if (!SpawnedPet)
	{
		MF_LOG_ERROR(LogMFInventory, TEXT("SummonPet: SpawnActor failed for '%s'."),
			*InstancePtr->AIConfigID.ToString());
		return nullptr;
	}

	// 先 ApplyPetConfig 铺底属性（含 InitAttributes 初始化），再用快照覆盖，
	// 确保还原的血量/属性不被初始值盖掉（顺序：技能/标签/感知/动画/AI → 属性快照）。
	SpawnedPet->ApplyPetConfig(Config);
	SpawnedPet->RestoreFromInstance(*InstancePtr);

	// 阵营 + 索敌：宠物 DataAsset 默认是中立/野生配置；召唤宠在此翻转为玩家阵营并改索敌目标。
	if (UAbilitySystemComponent* PetASC = SpawnedPet->GetAbilitySystemComponent())
	{
		UMFFactionStatics::SetFaction(PetASC, SummonedPetTeamTags);
	}
	if (UMFRadarSensingComponent* Radar = SpawnedPet->FindComponentByClass<UMFRadarSensingComponent>())
	{
		Radar->TargetTags = SummonedPetTargetTags;
		Radar->ForceScan();
	}

	if (AMFPetAIController* AIC = Cast<AMFPetAIController>(SpawnedPet->GetController()))
	{
		AIC->RunStateTree(Config->StateTreeAsset);
	}
	else
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("SummonPet: '%s' has no AMFPetAIController — StateTree not started."),
			*InstancePtr->PetName);
	}

	ActivePetActors.Add(InstanceID, SpawnedPet);
	InstancePtr->bIsActive = true;

	// 订阅该宠物阵亡，附带 InstanceID 以便回调定位实例。
	// AttributeSet 随 Actor 销毁而失效，因此召回/阵亡销毁 Actor 时绑定自动失效，无需手动解绑。
	if (UAbilitySystemComponent* PetASC = SpawnedPet->GetAbilitySystemComponent())
	{
		if (const UMFAttributeSetBase* Set = PetASC->GetSet<UMFAttributeSetBase>())
		{
			const_cast<UMFAttributeSetBase*>(Set)->OnDeath.AddUObject(
				this, &UMFInventoryComponent::HandlePetDied, InstanceID);
		}
	}

	MF_LOG(LogMFInventory, TEXT("SummonPet: '%s' (%s) spawned at %s."),
		*InstancePtr->PetName, *InstancePtr->AIConfigID.ToString(), *Location.ToString());
	OnPetRosterChanged.Broadcast();

	return SpawnedPet;
}

void UMFInventoryComponent::RecallPet(FGuid InstanceID)
{
	TWeakObjectPtr<AMFPetBase>* ActorPtr = ActivePetActors.Find(InstanceID);
	if (!ActorPtr || !ActorPtr->IsValid())
	{
		MF_LOG_WARNING(LogMFInventory, TEXT("RecallPet: Actor not found or already destroyed."));
		ActivePetActors.Remove(InstanceID);
		return;
	}

	AMFPetBase* Pet = ActorPtr->Get();

	// 刷新快照（只更新属性，保留 Level/Exp/Name 等）
	if (FMFPetInstance* InstancePtr = FindPetMutable(InstanceID))
	{
		Pet->SerializeToInstance(*InstancePtr);
		InstancePtr->bIsActive = false;
	}

	Pet->Destroy();
	ActivePetActors.Remove(InstanceID);

	MF_LOG(LogMFInventory, TEXT("RecallPet: Pet recalled and snapshot updated."));
	OnPetRosterChanged.Broadcast();
}

// ============================================================
// 宠物 — 阵亡 / 复活
// ============================================================

void UMFInventoryComponent::HandlePetDied(FGuid InstanceID)
{
	FMFPetInstance* Inst = FindPetMutable(InstanceID);
	if (!Inst || Inst->bIsDead)
	{
		return;   // 实例已移除或已在复活中（防重入）
	}

	// 从战场移除（延迟到下一帧销毁，避免在 OnDeath 广播中销毁自身 ASC）
	DestroyPetActorDeferred(InstanceID);

	Inst->bIsActive          = false;
	Inst->bIsDead            = true;
	Inst->ReviveTimeRemaining = PetReviveDuration;

	MF_LOG(LogMFInventory, TEXT("HandlePetDied: '%s' down — reviving in %.0fs."),
		*Inst->PetName, PetReviveDuration);

	if (PetReviveDuration <= 0.f)
	{
		ReviveSinglePet(*Inst);   // 立即可再召唤
	}
	else
	{
		EnsureReviveTickerRunning();
	}

	OnPetRosterChanged.Broadcast();
}

void UMFInventoryComponent::OnReviveTick()
{
	bool bAnyStillReviving = false;
	bool bAnyRevivedNow    = false;

	for (FMFPetInstance& Pet : PetSlots)
	{
		if (!Pet.bIsDead) continue;

		Pet.ReviveTimeRemaining = FMath::Max(0.f, Pet.ReviveTimeRemaining - 1.f);
		if (Pet.ReviveTimeRemaining <= 0.f)
		{
			ReviveSinglePet(Pet);
			bAnyRevivedNow = true;
			MF_LOG(LogMFInventory, TEXT("OnReviveTick: '%s' revived — ready to summon."), *Pet.PetName);
		}
		else
		{
			bAnyStillReviving = true;
		}
	}

	OnPetReviveTick.Broadcast();           // UI 刷新读秒数字
	if (bAnyRevivedNow)
	{
		OnPetRosterChanged.Broadcast();    // 状态切换，刷新槽位
	}

	if (!bAnyStillReviving)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReviveTickHandle);
		}
	}
}

void UMFInventoryComponent::EnsureReviveTickerRunning()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!World->GetTimerManager().IsTimerActive(ReviveTickHandle))
	{
		World->GetTimerManager().SetTimer(
			ReviveTickHandle, this, &UMFInventoryComponent::OnReviveTick,
			1.f, /*bLoop=*/true, /*FirstDelay=*/1.f);
	}
}

void UMFInventoryComponent::ReviveSinglePet(FMFPetInstance& Instance) const
{
	Instance.bIsDead            = false;
	Instance.ReviveTimeRemaining = 0.f;

	// 回满血：把快照 Health 设回 MaxHealth（下次召唤 RestoreFromInstance 时生效）。
	// 不能依赖死亡瞬间快照（Health=0）。
	if (const float* MaxHP = Instance.AttributeSnapshot.Find(TEXT("MaxHealth")))
	{
		Instance.AttributeSnapshot.Add(TEXT("Health"), *MaxHP);
	}
}

void UMFInventoryComponent::DestroyPetActorDeferred(FGuid InstanceID)
{
	TWeakObjectPtr<AMFPetBase> Weak;
	if (const TWeakObjectPtr<AMFPetBase>* ActorPtr = ActivePetActors.Find(InstanceID))
	{
		Weak = *ActorPtr;
	}
	ActivePetActors.Remove(InstanceID);

	if (!Weak.IsValid()) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([Weak]()
		{
			if (Weak.IsValid())
			{
				Weak->Destroy();
			}
		});
	}
}

// ============================================================
// 宠物 — 查询
// ============================================================

const FMFPetInstance* UMFInventoryComponent::FindPet(FGuid InstanceID) const
{
	for (const FMFPetInstance& Pet : PetSlots)
	{
		if (Pet.InstanceID == InstanceID) return &Pet;
	}
	return nullptr;
}

FMFPetInstance* UMFInventoryComponent::FindPetMutable(FGuid InstanceID)
{
	for (FMFPetInstance& Pet : PetSlots)
	{
		if (Pet.InstanceID == InstanceID) return &Pet;
	}
	return nullptr;
}

TArray<FMFPetInstance> UMFInventoryComponent::GetActivePets() const
{
	TArray<FMFPetInstance> Active;
	for (const FMFPetInstance& Pet : PetSlots)
	{
		if (Pet.bIsActive) Active.Add(Pet);
	}
	return Active;
}

TArray<AMFPetBase*> UMFInventoryComponent::GetActivePetActors() const
{
	TArray<AMFPetBase*> Result;
	Result.Reserve(ActivePetActors.Num());
	for (const auto& Pair : ActivePetActors)
	{
		if (Pair.Value.IsValid())
		{
			Result.Add(Pair.Value.Get());
		}
	}
	return Result;
}

AMFPetBase* UMFInventoryComponent::GetActivePetActor(FGuid InstanceID) const
{
	const TWeakObjectPtr<AMFPetBase>* ActorPtr = ActivePetActors.Find(InstanceID);
	return (ActorPtr && ActorPtr->IsValid()) ? ActorPtr->Get() : nullptr;
}

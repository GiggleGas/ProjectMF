// Copyright ProjectMF. All Rights Reserved.

#include "MFInventoryComponent.h"
#include "MFItemStatics.h"
#include "MFItemSettings.h"
#include "MFLootSubsystem.h"
#include "MFAIRegistry.h"
#include "MFPetConfig.h"
#include "MFPetBase.h"
#include "MFPetAIController.h"
#include "MFAttributeSetBase.h"
#include "MFFactionStatics.h"
#include "MFGameplayTags.h"
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
		if (Resources[i].ItemID <= 0 || Resources[i].Count <= 0) continue; // 跳过空格
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

void UMFInventoryComponent::InitResourceSlots()
{
	if (MaxResourceSlots <= 0)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("InitResourceSlots: MaxResourceSlots<=0（未在 PlayerConfig 配置），回退默认 20。"));
		MaxResourceSlots = 20;
	}
	// 预填固定格数，空格 = 默认构造（ItemID=0, Count=0）。下标即格位。
	ResourceSlots.Empty(MaxResourceSlots);
	ResourceSlots.SetNum(MaxResourceSlots);

	// 通知 UI 按新容量重建格子（HUD 可能早于本函数初始化，需据此补建 N 格空位）。
	OnInventoryChanged.Broadcast();
}

int32 UMFInventoryComponent::AddResource(int32 ItemID, int32 Count)
{
	if (Count <= 0 || ItemID <= 0)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddResource: invalid args (ID=%d, Count=%d)."), ItemID, Count);
		return 0;
	}

	const FMFItemDef* Def = UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID);
	if (!Def)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddResource: ItemID %d not found in database."), ItemID);
		return 0;
	}

	// 可叠加类才进背包格：资源 + 消耗品（打造产出）。
	if (Def->ItemType != EMFItemType::Resource && Def->ItemType != EMFItemType::Consumable)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddResource: %d 非可叠加类（Resource/Consumable），拒绝。"), ItemID);
		return 0;
	}

	const int32 MaxStack = FMath::Max(Def->MaxStackSize, 1);
	int32 Remaining = Count;

	// 第一遍：填所有未满的同种格（从前往后）。
	for (FMFInventorySlot& Slot : ResourceSlots)
	{
		if (Remaining <= 0) break;
		if (Slot.ItemID == ItemID && Slot.Count < MaxStack)
		{
			const int32 Move = FMath::Min(MaxStack - Slot.Count, Remaining);
			Slot.Count += Move;
			Remaining  -= Move;
		}
	}

	// 第二遍：占空格新建堆。
	for (FMFInventorySlot& Slot : ResourceSlots)
	{
		if (Remaining <= 0) break;
		if (Slot.ItemID <= 0 || Slot.Count <= 0)
		{
			Slot.ItemID = ItemID;
			Slot.Count  = FMath::Min(MaxStack, Remaining);
			Remaining  -= Slot.Count;
		}
	}

	const int32 ActualAdded = Count - Remaining;
	if (ActualAdded > 0)
	{
		MF_LOG(LogMFInventory, TEXT("AddResource: +%d #%d (total: %d)."),
			ActualAdded, ItemID, GetResourceCount(ItemID));
		OnInventoryChanged.Broadcast();
	}
	// Remaining>0 表示背包满、剩余未入包（返回值 < Count，调用方据此弹回/留地）。

	return ActualAdded;
}

bool UMFInventoryComponent::RemoveResource(int32 ItemID, int32 Count)
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

		if (ResourceSlots[i].Count <= 0)
		{
			// 置空保留格位（固定索引数组，不塌缩）。
			ResourceSlots[i].ItemID = 0;
			ResourceSlots[i].Count  = 0;
		}
	}

	MF_LOG(LogMFInventory, TEXT("RemoveResource: -%d #%d (remaining: %d)."),
		Count, ItemID, GetResourceCount(ItemID));
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UMFInventoryComponent::GetResourceCount(int32 ItemID) const
{
	int32 Total = 0;
	for (const FMFInventorySlot& Slot : ResourceSlots)
	{
		if (Slot.ItemID == ItemID) Total += Slot.Count;
	}
	return Total;
}

bool UMFInventoryComponent::HasResource(int32 ItemID, int32 Count) const
{
	return GetResourceCount(ItemID) >= Count;
}

bool UMFInventoryComponent::DropSlot(int32 SlotIndex)
{
	if (!ResourceSlots.IsValidIndex(SlotIndex)) return false;

	FMFInventorySlot& Slot = ResourceSlots[SlotIndex];
	if (Slot.ItemID <= 0 || Slot.Count <= 0) return false; // 空格

	// 掉回 owner 脚下（复用掉落管线，可再捡）。
	if (UWorld* World = GetWorld())
	{
		if (UMFLootSubsystem* Loot = World->GetSubsystem<UMFLootSubsystem>())
		{
			FMFLootResult Result;
			Result.ItemID = Slot.ItemID;
			Result.Count  = Slot.Count;
			const FVector Origin = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
			Loot->SpawnLoot({ Result }, Origin);
		}
	}

	Slot.ItemID = 0;
	Slot.Count  = 0;
	OnInventoryChanged.Broadcast();
	return true;
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

	// 阵营：野怪默认敌方，召唤宠在此翻转为玩家阵营。索敌方向由阵营自动判定（faction-auto），无需配置。
	if (UAbilitySystemComponent* PetASC = SpawnedPet->GetAbilitySystemComponent())
	{
		UMFFactionStatics::SetFaction(PetASC, SummonedPetTeamTags);
		// 召唤标记：StateTree 据此分流（受指令 + 走自动/手动模式）。Actor 销毁(召回/阵亡)时随之失效。
		PetASC->AddLooseGameplayTag(MFGameplayTags::Pet_Summoned);
	}
	// 翻阵营后强制一次扫描，立即按新阵营重算索敌。
	if (UMFRadarSensingComponent* Radar = SpawnedPet->FindComponentByClass<UMFRadarSensingComponent>())
	{
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

	// 濒死系统：真死（永久损失）/ 复活回场 信号，绑定时带 InstanceID。
	SpawnedPet->OnTrueDeath.AddUObject(this, &UMFInventoryComponent::HandlePetTrueDeath, InstanceID);
	SpawnedPet->OnRevived.AddUObject(this, &UMFInventoryComponent::HandlePetRevived, InstanceID);

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
	// HP→0：进入濒死（不销毁、不自动复活）。Actor 保留在 ActivePetActors——
	// 全灭判负只在"全部真死"时触发（濒死宠仍算在场，留抢救窗口）。
	FMFPetInstance* Inst = FindPetMutable(InstanceID);
	if (!Inst || Inst->bIsDead) return;   // 已真死 / 已移除

	Inst->bIsActive = false;

	if (const TWeakObjectPtr<AMFPetBase>* ActorPtr = ActivePetActors.Find(InstanceID))
	{
		if (AMFPetBase* Pet = ActorPtr->Get())
		{
			Pet->EnterDowned();   // 起 bleed-out；归零未救 → OnTrueDeath（已在 SummonPet 绑定）
		}
	}

	MF_LOG(LogMFInventory, TEXT("HandlePetDied: '%s' 濒死。"), *Inst->PetName);
	OnPetRosterChanged.Broadcast();
}

void UMFInventoryComponent::HandlePetTrueDeath(FGuid InstanceID)
{
	// 濒死读条耗尽未救 → 真死：销毁 Actor + 永久损失（墓碑逻辑随 v3 元层补）。
	FMFPetInstance* Inst = FindPetMutable(InstanceID);
	if (Inst)
	{
		Inst->bIsDead   = true;
		Inst->bIsActive = false;
	}
	DestroyPetActorDeferred(InstanceID);   // 移出 ActivePetActors + 下一帧销毁
	MF_LOG(LogMFInventory, TEXT("HandlePetTrueDeath: '%s' 真死（永久损失）。"),
		Inst ? *Inst->PetName : TEXT("?"));
	OnPetRosterChanged.Broadcast();
}

void UMFInventoryComponent::HandlePetRevived(FGuid InstanceID)
{
	// 复活读条完成 → 回出战。
	if (FMFPetInstance* Inst = FindPetMutable(InstanceID))
	{
		Inst->bIsActive = true;
		MF_LOG(LogMFInventory, TEXT("HandlePetRevived: '%s' 复活回场。"), *Inst->PetName);
	}
	OnPetRosterChanged.Broadcast();
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

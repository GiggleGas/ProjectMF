// Copyright ProjectMF. All Rights Reserved.

#include "MFInventoryComponent.h"
#include "MFItemDatabase.h"
#include "MFLog.h"

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

	// 优先填满已有格子
	const int32 SlotIdx = FindResourceSlotIndex(ItemID);
	if (SlotIdx != INDEX_NONE)
	{
		FMFInventorySlot& Slot = ResourceSlots[SlotIdx];
		const int32 Space = MaxStack - Slot.Count;
		const int32 Added = FMath::Min(Space, Remaining);
		Slot.Count += Added;
		Remaining  -= Added;
	}

	// 剩余量：新建格子
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

	// 从末尾向前扣除（避免正向遍历删除引起下标偏移）
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
		if (Slot.ItemID == ItemID)
		{
			Total += Slot.Count;
		}
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
		if (ResourceSlots[i].ItemID == ItemID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// ============================================================
// 宠物
// ============================================================

FGuid UMFInventoryComponent::AddPet(FName PetItemID, const FString& PetName)
{
	if (PetItemID.IsNone())
	{
		MF_LOG_WARNING(LogMFInventory, TEXT("AddPet: PetItemID is None."));
		return FGuid();
	}

	if (MaxPetSlots > 0 && PetSlots.Num() >= MaxPetSlots)
	{
		MF_LOG_WARNING(LogMFInventory,
			TEXT("AddPet: Pet roster full (%d/%d)."), PetSlots.Num(), MaxPetSlots);
		return FGuid();
	}

	// 昵称：优先用参数，为空时取 DisplayName
	FString FinalName = PetName;
	if (FinalName.IsEmpty() && ItemDatabase)
	{
		if (const FMFItemDef* Def = ItemDatabase->FindItem(PetItemID))
		{
			FinalName = Def->DisplayName.ToString();
		}
	}
	if (FinalName.IsEmpty())
	{
		FinalName = PetItemID.ToString();
	}

	FMFPetInstance NewPet;
	NewPet.InstanceID = FGuid::NewGuid();
	NewPet.PetItemID  = PetItemID;
	NewPet.PetName    = FinalName;
	NewPet.Level      = 1;
	NewPet.Experience = 0;
	NewPet.bIsActive  = false;

	PetSlots.Add(NewPet);

	MF_LOG(LogMFInventory, TEXT("AddPet: '%s' (%s) registered. Total: %d."),
		*FinalName, *PetItemID.ToString(), PetSlots.Num());
	OnPetRosterChanged.Broadcast();

	return NewPet.InstanceID;
}

const FMFPetInstance* UMFInventoryComponent::FindPet(FGuid InstanceID) const
{
	for (const FMFPetInstance& Pet : PetSlots)
	{
		if (Pet.InstanceID == InstanceID)
		{
			return &Pet;
		}
	}
	return nullptr;
}

void UMFInventoryComponent::SetPetActive(FGuid InstanceID, bool bActive)
{
	for (FMFPetInstance& Pet : PetSlots)
	{
		if (Pet.InstanceID == InstanceID)
		{
			Pet.bIsActive = bActive;
			MF_LOG(LogMFInventory, TEXT("SetPetActive: '%s' -> %s."),
				*Pet.PetName, bActive ? TEXT("Active") : TEXT("Inactive"));
			OnPetRosterChanged.Broadcast();
			return;
		}
	}

	MF_LOG_WARNING(LogMFInventory, TEXT("SetPetActive: InstanceID not found."));
}

TArray<FMFPetInstance> UMFInventoryComponent::GetActivePets() const
{
	TArray<FMFPetInstance> Active;
	for (const FMFPetInstance& Pet : PetSlots)
	{
		if (Pet.bIsActive)
		{
			Active.Add(Pet);
		}
	}
	return Active;
}

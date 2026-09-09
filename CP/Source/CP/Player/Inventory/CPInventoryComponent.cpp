#include "Player/Inventory/CPInventoryComponent.h"
#include "Player/Inventory/CPUsableItem.h"

UCPInventoryComponent::UCPInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Slots.SetNum(NumSlots);
}

bool UCPInventoryComponent::StoreItem(FName ItemCode, int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	for (FCPInventorySlot& Slot : Slots)
	{
		if (Slot.Item.ItemCode == ItemCode)
		{
			Slot.Count += Count;
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UCPInventoryComponent::RemoveItem(FName ItemCode, int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	for (FCPInventorySlot& Slot : Slots)
	{
		if (Slot.Item.ItemCode == ItemCode)
		{
			if (Slot.Count < Count)
			{
				return false;
			}

			Slot.Count -= Count;
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	return false;
}

int32 UCPInventoryComponent::GetItemCount(FName ItemCode) const
{
	for (const FCPInventorySlot& Slot : Slots)
	{
		if (Slot.Item.ItemCode == ItemCode)
		{
			return Slot.Count;
		}
	}

	return 0;
}

bool UCPInventoryComponent::UseSlotItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		return false;
	}

	FCPInventorySlot& Slot = Slots[SlotIndex];

	if (Slot.Item.UseEffectClass)
	{
		if (UObject* EffectInstance = NewObject<UObject>(this, Slot.Item.UseEffectClass))
		{
			if (ICPUsableItem* Usable = Cast<ICPUsableItem>(EffectInstance))
			{
				Usable->UseItem(GetOwner(), Slot.Item);
			}
		}
	}

	Slot.Count -= 1;
	OnInventoryChanged.Broadcast();

	return true;
}

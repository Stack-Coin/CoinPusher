#include "Player/Inventory/CPInventoryComponent.h"
#include "Player/Inventory/CPUsableItem.h"
#include "Player/CPPlayerCharacter.h"
#include "CoinPusher/CPCoinPusher.h"
#include "Roulette/CPRoulette.h"
#include "Log/CPLogCategories.h"

UCPInventoryComponent::UCPInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Slots.SetNum(NumSlots);
}

void UCPInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(GetOwner()))
	{
		if (ACPCoinPusher* CoinPusher = PlayerCharacter->GetCoinPusher())
		{
			if (ACPRoulette* LinkedRoulette = CoinPusher->GetLinkedRoulette())
			{
				LinkedRoulette->OnPickedUp.AddDynamic(this, &UCPInventoryComponent::HandleRoulettePickedUp);
			}
		}
	}
}

void UCPInventoryComponent::HandleRoulettePickedUp(FName ItemID, int32 Count)
{
	UE_LOG(LogPlayer, Warning, TEXT("Inventory <- OnDropped Broadcast"));
	StoreItem(ItemID, Count);
}

bool UCPInventoryComponent::StoreItem(FName ItemCode, int32 Count)
{
	UE_LOG(LogPlayer, Warning, TEXT("StoreItem called - ItemCode: %s, Count: %d"), *ItemCode.ToString(), Count);

	if (Count <= 0)
	{
		return false;
	}

	for (FCPInventorySlot& Slot : Slots)
	{
		if (Slot.Item.ItemCode == ItemCode)
		{
			Slot.Count += Count;
			UE_LOG(LogPlayer, Warning, TEXT("StoreItem updated - ItemCode: %s, Updated Count: %d"), *ItemCode.ToString(), Slot.Count);
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

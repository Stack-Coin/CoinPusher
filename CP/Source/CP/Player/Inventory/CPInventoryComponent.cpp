#include "Player/Inventory/CPInventoryComponent.h"
#include "Player/CPPlayerCharacter.h"
#include "CoinPusher/CPCoinPusher.h"
#include "Roulette/CPRoulette.h"
#include "Log/CPLogCategories.h"
#include "Datatables/CPItemData.h"
#include "Engine/DataTable.h"

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
		CoinPusher = PlayerCharacter->GetCoinPusher();

		if (ACPRoulette* LinkedRoulette = CoinPusher ? CoinPusher->GetLinkedRoulette() : nullptr)
		{
			LinkedRoulette->OnPickedUp.AddDynamic(this, &UCPInventoryComponent::HandleRoulettePickedUp);
		}
	}
}

void UCPInventoryComponent::HandleRoulettePickedUp(FName ItemID, int32 Count)
{
	UE_LOG(LogPlayer, Warning, TEXT("Inventory <- OnDropped Broadcast"));
	StoreItem(ItemID, Count);
}

const FItemData* UCPInventoryComponent::FindItemData(FName ItemID) const
{
	UDataTable* ItemDataTable = CoinPusher ? CoinPusher->GetItemDataTable() : nullptr;
	return ItemDataTable
		? ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UCPInventoryComponent::FindItemData"))
		: nullptr;
}

bool UCPInventoryComponent::StoreItem(FName ItemCode, int32 Count)
{
	UE_LOG(LogPlayer, Warning, TEXT("StoreItem called - ItemCode: %s, Count: %d"), *ItemCode.ToString(), Count);

	if (Count <= 0 || ItemCode.IsNone())
	{
		return false;
	}

	// 이미 같은 아이템을 담은 슬롯이 있으면 그 슬롯 개수만 증가
	for (FCPInventorySlot& Slot : Slots)
	{
		if (Slot.ItemID == ItemCode)
		{
			Slot.Count += Count;
			UE_LOG(LogPlayer, Warning, TEXT("StoreItem updated - ItemCode: %s, Updated Count: %d"), *ItemCode.ToString(), Slot.Count);
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	// 없으면 ItemDataTable에 그 ItemCode 행이 실제로 존재하는지 확인한 뒤 빈 슬롯을 새로 채운다
	// (슬롯은 ItemID만 들고 있고, 이름/아이콘 등 나머지는 항상 ItemDataTable에서 조회한다)
	if (!FindItemData(ItemCode))
	{
		return false;
	}

	for (FCPInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Slot.ItemID = ItemCode;
			Slot.Count = Count;
			UE_LOG(LogPlayer, Warning, TEXT("StoreItem stored in new slot - ItemCode: %s, Count: %d"), *ItemCode.ToString(), Count);
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
		if (Slot.ItemID == ItemCode)
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
		if (Slot.ItemID == ItemCode)
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
	const FName UsedItemID = Slot.ItemID;                // Count 감소 전에 미리 저장
	const int32 BroadcastCount = Slot.UseBroadcastCount;  // 슬롯별 BP 지정값

	Slot.Count -= 1;
	OnInventoryChanged.Broadcast();
	OnItemUsed.Broadcast(UsedItemID, BroadcastCount);

	return true;
}

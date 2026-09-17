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

	// 이미 같은 아이템을 담은 슬롯(또는 이 아이템 전용으로 예약된, ItemID는 지정돼 있지만 Count가
	// 0인 슬롯)이 있으면 그 슬롯 개수만 증가
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

	// ItemID가 아예 비어있는(특정 아이템으로 예약되지 않은) 슬롯만 새로 채운다 - 다른 아이템 전용으로
	// 예약된 슬롯(ItemID는 지정돼 있지만 Count가 0인 상태, IsEmpty()는 true)은 여기서 건드리지 않는다.
	// 그런 슬롯에 들어갈 자격이 있는 아이템은 위 루프에서 이미 ItemID가 일치해 처리됐어야 한다
	for (FCPInventorySlot& Slot : Slots)
	{
		if (Slot.ItemID.IsNone())
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

	if (CoinPusher && !CoinPusher->CanUseItem(Slot.ItemID))
	{
		return false;
	}

	const FName UsedItemID = Slot.ItemID;                // Count 감소 전에 미리 저장
	const int32 BroadcastCount = Slot.UseBroadcastCount;  // 슬롯별 BP 지정값

	Slot.Count -= 1;
	OnInventoryChanged.Broadcast();
	OnItemUsed.Broadcast(UsedItemID, BroadcastCount);

	return true;
}

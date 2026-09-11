#pragma once

#include "CoreMinimal.h"
#include "CPInventoryTypes.generated.h"

/** 슬롯은 ItemID만 들고 있고, 이름/아이콘/효과 등 나머지 정보는 전부 ItemDataTable(FItemData)에서
 *  조회한다 - 슬롯과 마스터 데이터가 따로 노는 것을 막기 위함 (ID 하나만 원본으로 유지) */
USTRUCT(BlueprintType)
struct FCPInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FName ItemID;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 Count = 0;

	/** 이 슬롯을 사용(UseSlotItem)했을 때 OnItemUsed로 브로드캐스트할 개수.
	 *  실제 인벤토리에서 소모되는 수량(항상 1)과는 별개 - 슬롯별로 BP에서 직접 지정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 UseBroadcastCount = 1;

	bool IsEmpty() const { return Count <= 0 || ItemID.IsNone(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPInventoryChanged);

/** UseSlotItem()이 슬롯을 사용했을 때 Broadcast - ItemID는 사용된 아이템의 코드,
 *  Count는 이번에 브로드캐스트하는 개수 - 슬롯별로 BP에서 지정한 FCPInventorySlot::UseBroadcastCount
 *  값이며, 실제로 소모된 인벤토리 수량(항상 1)과는 다를 수 있음 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPInventoryItemUsed, FName, ItemID, int32, Count);

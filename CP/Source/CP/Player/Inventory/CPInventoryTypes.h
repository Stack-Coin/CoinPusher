#pragma once

#include "CoreMinimal.h"
#include "Player/CPItemTypes.h"
#include "CPInventoryTypes.generated.h"

USTRUCT(BlueprintType)
struct FCPInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FCPItemData Item;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 Count = 0;

	bool IsEmpty() const { return Count <= 0 || Item.ItemCode.IsNone(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPInventoryChanged);

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Player/Inventory/CPInventoryTypes.h"
#include "CPSlotInventory.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPSlotInventory : public UInterface
{
	GENERATED_BODY()
};

class ICPSlotInventory
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Inventory")
	virtual bool StoreItem(FName ItemCode, int32 Count) = 0;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	virtual bool RemoveItem(FName ItemCode, int32 Count) = 0;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	virtual int32 GetItemCount(FName ItemCode) const = 0;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	virtual bool UseSlotItem(int32 SlotIndex) = 0;

	virtual const TArray<FCPInventorySlot>& GetSlots() const = 0;
};

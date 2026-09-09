#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/Inventory/CPSlotInventory.h"
#include "CPInventoryComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CP_API UCPInventoryComponent : public UActorComponent, public ICPSlotInventory
{
	GENERATED_BODY()

public:

	UCPInventoryComponent();

protected:

	static constexpr int32 NumSlots = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta = (EditFixedSize))
	TArray<FCPInventorySlot> Slots;

public:

	virtual bool StoreItem(FName ItemCode, int32 Count) override;

	virtual bool RemoveItem(FName ItemCode, int32 Count) override;

	virtual int32 GetItemCount(FName ItemCode) const override;

	virtual bool UseSlotItem(int32 SlotIndex) override;

	virtual const TArray<FCPInventorySlot>& GetSlots() const override { return Slots; }

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnCPInventoryChanged OnInventoryChanged;
};

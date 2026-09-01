// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Player/CPItemTypes.h"
#include "CPItemInventory.generated.h"

/**
 *  CPItemInventory
 *  Implemented by whoever can own items (the player). Item actors add themselves to the
 *  inventory purely through this interface, never by writing to a concrete player's array.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPItemInventory : public UInterface
{
	GENERATED_BODY()
};

class ICPItemInventory
{
	GENERATED_BODY()

public:

	/** Adds an item to the inventory. Duplicate ItemCodes are allowed - stacking is just multiple entries */
	UFUNCTION(BlueprintCallable, Category="Item")
	virtual void AddOwnedItem(const FCPItemData& ItemData) = 0;

	/** Returns true if at least one item with the given code is owned */
	UFUNCTION(BlueprintCallable, Category="Item")
	virtual bool HasItem(FName ItemCode) const = 0;

	/** Returns how many items with the given code are owned */
	UFUNCTION(BlueprintCallable, Category="Item")
	virtual int32 GetItemCount(FName ItemCode) const = 0;

	/** Returns every currently owned item, in acquisition order */
	virtual const TArray<FCPItemData>& GetOwnedItems() const = 0;
};

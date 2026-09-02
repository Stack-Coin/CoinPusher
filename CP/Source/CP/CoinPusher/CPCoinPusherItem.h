// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPCoinPusherItem.generated.h"

class ACPDropZone;

/**
 *
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPCoinPusherItem : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Common interface for anything ACPDispenser can spawn and ACPDropZone can collect
 *  (coins, prize items, ...). Lets ACPDropZone react to a collected object without
 *  knowing its concrete type.
 */
class ICPCoinPusherItem
{
	GENERATED_BODY()

public:

	/** Called by ACPDropZone when this item falls into it. Implementations report themselves
	 *  to DropZone (e.g. add to the coin count, record an item code) and clean themselves up */
	UFUNCTION(BlueprintCallable, Category="CoinPusher")
	virtual void OnDroppedInZone(ACPDropZone* DropZone) = 0;

};

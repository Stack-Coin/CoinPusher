// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPCoinWallet.generated.h"

/**
 *  CPCoinWallet
 *  Simple interface for anything that can hold and spend coins. Coin actors only ever
 *  talk to this interface, never to a concrete player class.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPCoinWallet : public UInterface
{
	GENERATED_BODY()
};

class ICPCoinWallet
{
	GENERATED_BODY()

public:

	/** Adds Amount to the current coin balance */
	UFUNCTION(BlueprintCallable, Category="Coin")
	virtual void AddCoin(int32 Amount) = 0;

	/** Returns the current coin balance */
	UFUNCTION(BlueprintCallable, Category="Coin")
	virtual int32 GetCoinAmount() const = 0;

	/** Returns true if the current coin balance is at least Amount */
	UFUNCTION(BlueprintCallable, Category="Coin")
	virtual bool HasEnoughCoin(int32 Amount) const = 0;

	/** Attempts to spend Amount coins. Deducts and returns true on success; leaves the balance
	 *  unchanged and returns false if there aren't enough coins */
	UFUNCTION(BlueprintCallable, Category="Coin")
	virtual bool TrySpendCoin(int32 Amount) = 0;
};

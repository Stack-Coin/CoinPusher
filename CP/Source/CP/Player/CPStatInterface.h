// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Player/CPStatTypes.h"
#include "CPStatInterface.generated.h"

/**
 *  CPStatInterface
 *  Common interface external systems (items, augments, buffs, etc.) use to read and modify
 *  a character's stats, instead of writing to the character's internal variables directly.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPStatInterface : public UInterface
{
	GENERATED_BODY()
};

class ICPStatInterface
{
	GENERATED_BODY()

public:

	/** Adds Delta to the current value of the given stat */
	UFUNCTION(BlueprintCallable, Category="Stats")
	virtual void ModifyStat(ECPStatType StatType, float Delta) = 0;

	/** Sets the given stat to an absolute value */
	UFUNCTION(BlueprintCallable, Category="Stats")
	virtual void SetStat(ECPStatType StatType, float NewValue) = 0;

	/** Returns the current value of the given stat */
	UFUNCTION(BlueprintCallable, Category="Stats")
	virtual float GetStat(ECPStatType StatType) const = 0;
};

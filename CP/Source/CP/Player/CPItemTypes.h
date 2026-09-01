// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPItemEffect.h"
#include "CPItemTypes.generated.h"

/**
 *  Data for a single item. Assigning EffectClass is the extension point for item effects -
 *  new effects are added as new UCPItemEffect subclasses, without touching pickup/inventory code.
 */
USTRUCT(BlueprintType)
struct FCPItemData
{
	GENERATED_BODY()

	/** Unique item identifier, e.g. "ITEM_SPEED_001" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FName ItemCode;

	/** Display name, e.g. "낡은 운동화" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FText ItemName;

	/** Flavor/description text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FText Description;

	/** Effect applied when this item is acquired. None = no effect (the default for every current item) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSubclassOf<UCPItemEffect> EffectClass;
};

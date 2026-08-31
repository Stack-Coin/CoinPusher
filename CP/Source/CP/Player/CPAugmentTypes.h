// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPStatTypes.h"
#include "CPAugmentTypes.generated.h"

/** A single stat change applied by an augment, expressed as a percent of the stat's current value */
USTRUCT(BlueprintType)
struct FCPAugmentEffect
{
	GENERATED_BODY()

	/** Which stat this effect changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Augment")
	ECPStatType StatType = ECPStatType::AttackPower;

	/** Percent change to apply to the stat's current value. e.g. 5.0 = +5%, -5.0 = -5% */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Augment", meta = (Units = "Percent"))
	float PercentChange = 0.0f;
};

/**
 *  Data for a single selectable augment. New augments are added purely as data
 *  (an entry in an augment pool array) without any changes to the selection code.
 */
USTRUCT(BlueprintType)
struct FCPAugmentData
{
	GENERATED_BODY()

	/** Name shown on the augment card */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Augment")
	FText AugmentName;

	/** Effect description shown on the augment card */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Augment")
	FText Description;

	/** All stat changes this augment applies when selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Augment")
	TArray<FCPAugmentEffect> Effects;
};

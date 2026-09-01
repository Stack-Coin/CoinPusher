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

	/** Percent change to apply to the stat's current value. e.g. 5.0 = +5%, -5.0 = -5% (NOT the resulting 105/95) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Augment",
		meta = (Units = "Percent", DisplayName = "증가/감소율 (%)",
		ToolTip = "증가면 양수(예: 10 = +10%), 감소면 음수(예: -20 = -20%)를 입력하세요.\n110이나 80처럼 변경 후 최종값을 입력하는 것이 아닙니다."))
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

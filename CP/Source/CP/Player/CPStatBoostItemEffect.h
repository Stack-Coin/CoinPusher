// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPItemEffect.h"
#include "Player/CPStatTypes.h"
#include "CPStatBoostItemEffect.generated.h"

/**
 *  UCPStatBoostItemEffect
 *  Growth item effect: adds Amount to the target's StatToBoost when acquired (e.g. +10 AttackPower).
 *  Assign this (or a Blueprint child with StatToBoost/Amount preset) to a FCPItemData::EffectClass.
 */
UCLASS(meta = (DisplayName = "Stat Boost"))
class CP_API UCPStatBoostItemEffect : public UCPItemEffect
{
	GENERATED_BODY()

public:

	// ~begin UCPItemEffect

	virtual void ApplyEffect(TScriptInterface<ICPStatInterface> Target) override;

	// ~end UCPItemEffect

protected:

	/** Stat this item increases when acquired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	ECPStatType StatToBoost = ECPStatType::AttackPower;

	/** Amount added to StatToBoost */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float Amount = 10.0f;
};

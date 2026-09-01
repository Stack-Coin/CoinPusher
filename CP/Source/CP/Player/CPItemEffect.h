// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPStatInterface.h"
#include "CPItemEffect.generated.h"

/**
 *  Base class for an item's effect. Every current item intentionally uses no effect
 *  (FCPItemData::EffectClass = None). New effects (a move speed buff, an attack power buff,
 *  a heal, etc.) are added as new subclasses that override ApplyEffect - the item pickup and
 *  inventory code never has to change to support a new effect.
 */
UCLASS(Abstract)
class CP_API UCPItemEffect : public UObject
{
	GENERATED_BODY()

public:

	/** Applies this item's effect to the target. Base implementation intentionally does nothing */
	UFUNCTION(BlueprintCallable, Category="Item")
	virtual void ApplyEffect(TScriptInterface<ICPStatInterface> Target);
};

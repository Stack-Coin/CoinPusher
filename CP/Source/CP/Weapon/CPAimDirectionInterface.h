// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPAimDirectionInterface.generated.h"

/**
 *  CPAimDirectionProvider
 *  Optional interface for a weapon's wielder to supply an aim direction other than its own forward vector
 *  (e.g. a top-down character aiming at the mouse cursor). Weapons never cast to a concrete Character class
 *  to get this - they cast to this interface instead, so it stays optional and Actor-agnostic.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPAimDirectionProvider : public UInterface
{
	GENERATED_BODY()
};

class ICPAimDirectionProvider
{
	GENERATED_BODY()

public:

	/** Returns the world-space direction this wielder is currently aiming along */
	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual FVector GetAimDirection() const = 0;
};

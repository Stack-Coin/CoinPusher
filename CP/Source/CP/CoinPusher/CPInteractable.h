// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPInteractable.generated.h"

/**
 *
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Simple interface to allow CoinPusher actors to be interacted with by the player without exposing their internals.
 */
class ICPInteractable
{
	GENERATED_BODY()

public:

	/** Triggers an interaction by the provided Actor */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual void Interact(AActor* Interactor) = 0;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPInteractor.generated.h"

/**
 *  CPInteractor
 *  Implemented by whoever can interact with ICPInteractable Actors (the player).
 *  Range-based interactables (e.g. items) register/unregister themselves through this
 *  interface as the interactor enters/leaves their range, so an interactable never needs
 *  to know about a concrete player class.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPInteractor : public UInterface
{
	GENERATED_BODY()
};

class ICPInteractor
{
	GENERATED_BODY()

public:

	/** Called by an interactable when the interactor enters its interaction range */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual void RegisterInteractable(AActor* Interactable) = 0;

	/** Called by an interactable when the interactor leaves its interaction range */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual void UnregisterInteractable(AActor* Interactable) = 0;
};

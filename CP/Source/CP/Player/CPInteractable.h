// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPInteractable.generated.h"

/**
 *  CPInteractable
 *  Common interface for any Actor the player can interact with (coins, items, and later
 *  chests, NPCs, etc.), regardless of how the interaction is triggered - auto on overlap
 *  (coins) or an explicit key press once registered as the current target (items).
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPInteractable : public UInterface
{
	GENERATED_BODY()
};

class ICPInteractable
{
	GENERATED_BODY()

public:

	/** Executes the interaction (e.g. picking up a coin or item) */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual void Interact(AActor* Interactor) = 0;

	/** Returns true if this object can currently be interacted with. Defaults to always true */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual bool CanInteract(AActor* Interactor) const { return true; }

	/** Returns a display name for this interactable (e.g. for a future interact prompt UI) */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	virtual FText GetInteractableDisplayName() const { return FText::GetEmpty(); }
};

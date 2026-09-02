// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPMonsterSpawnInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPMonsterSpawnInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CP_API ICPMonsterSpawnInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** Toggles the Interactable Actor */
	//UFUNCTION(BlueprintCallable, Category = "Activatable")
	//virtual void ToggleInteraction(AActor* ActivationInstigator) = 0;

	/** Activates the Interactable Actor */
	UFUNCTION(BlueprintCallable, Category = "Activatable")
	virtual void ActivateInteraction(AActor* ActivationInstigator) = 0;

	/** Deactivates the Interactable Actor */
	//UFUNCTION(BlueprintCallable, Category = "Activatable")
	//virtual void DeactivateInteraction(AActor* ActivationInstigator) = 0;
};

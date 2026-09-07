// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CPTopDownPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UCPDebugWidget;

/**
 *  PlayerController for the top-down / quarter view action prototype.
 *  Adds its Input Mapping Contexts and exposes the mouse cursor's world location for attacks.
 */
UCLASS(abstract)
class CP_API ACPTopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts to add for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Shows/hides the F1 debug widget (player stats + collision visualization checkboxes). Only takes
	 *  effect for the local player who owns the keyboard/mouse - see IsUsingKeyboardAndMouse() */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleDebugWidgetAction;

	/** Widget Blueprint (inheriting UCPDebugWidget) shown/hidden by ToggleDebugWidgetAction (F1 by default) */
	UPROPERTY(EditDefaultsOnly, Category="Debug")
	TSubclassOf<UCPDebugWidget> DebugWidgetClass;

	/** Created lazily the first time the debug widget is toggled on, then reused */
	UPROPERTY()
	TObjectPtr<UCPDebugWidget> DebugWidgetInstance;

public:

	/** Constructor */
	ACPTopDownPlayerController();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Bound to ToggleDebugWidgetAction. Creates DebugWidgetInstance on first use, then shows/hides it.
	 *  No-ops for a player not on keyboard/mouse, or if DebugWidgetClass is unset */
	void ToggleDebugWidget(const FInputActionValue& Value);

public:

	/** Returns the world location under the mouse cursor, used to aim the basic attack */
	UFUNCTION(BlueprintCallable, Category="Input")
	bool GetCursorWorldLocation(FVector& OutWorldLocation) const;

	/** Returns true if this controller's platform user currently owns the default input device
	 *  (keyboard/mouse) - false for a player mapped to a gamepad instead (see ACPGameMode::BeginPlay).
	 *  ACPPlayerCharacter uses this to decide whether to aim with the cursor or the movement direction */
	UFUNCTION(BlueprintPure, Category="Input")
	bool IsUsingKeyboardAndMouse() const;
};

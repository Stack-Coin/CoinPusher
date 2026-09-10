// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CPTopDownPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UCPDebugWidget;
class UCPInGamePauseWidget;
class UCPEndingWidget;

class UCPCoinPusherCaptureWidget;

/**
 *  PlayerController for the top-down / quarter view action prototype.
 *  Adds its Input Mapping Contexts and exposes the mouse cursor's world location for attacks.
 */
UCLASS(abstract)
class CP_API ACPTopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	//CoinPusher ScreenCapture Widget
	UPROPERTY(EditDefaultsOnly, Category = "CoinPusher Picture-in-Picture")
	TSubclassOf<UCPCoinPusherCaptureWidget> CaptureWidgetClass;

	//ĸó ���� ����
	void SetupCaptureWidget();

	/** Input Mapping Contexts to add for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Shows/hides the F1 debug widget (player stats + collision visualization checkboxes). Works for
	 *  whichever local player triggers it, regardless of input device - each player gets their own
	 *  DebugWidgetInstance */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleDebugWidgetAction;

	/** Widget Blueprint (inheriting UCPDebugWidget) shown/hidden by ToggleDebugWidgetAction (F1 by default) */
	UPROPERTY(EditDefaultsOnly, Category="Debug")
	TSubclassOf<UCPDebugWidget> DebugWidgetClass;

	/** Created lazily the first time the debug widget is toggled on, then reused */
	UPROPERTY()
	TObjectPtr<UCPDebugWidget> DebugWidgetInstance;

	/** Opens/closes the InGamePause menu - map both the gamepad Menu button and keyboard Escape to this
	 *  same action in the Input Mapping Context, so either one toggles it (see TogglePauseMenu) */
	UPROPERTY(EditAnywhere, Category="Input|Pause")
	UInputAction* PauseAction;

	/** Moves the selection between the currently open menu's buttons (Axis2D, gamepad L-Stick) */
	UPROPERTY(EditAnywhere, Category="Input|Pause")
	UInputAction* MenuNavigateAction;

	/** Runs the currently selected button's action (gamepad A button) */
	UPROPERTY(EditAnywhere, Category="Input|Pause")
	UInputAction* MenuConfirmAction;

	/** Widget Blueprint (inheriting UCPInGamePauseWidget) shown/hidden by PauseAction */
	UPROPERTY(EditDefaultsOnly, Category="UI|Pause")
	TSubclassOf<UCPInGamePauseWidget> InGamePauseWidgetClass;

	/** Widget Blueprint (inheriting UCPEndingWidget) shown by ShowEndingResult */
	UPROPERTY(EditDefaultsOnly, Category="UI|Ending")
	TSubclassOf<UCPEndingWidget> EndingWidgetClass;

	/** Covers the whole screen (both local players' split-screen halves) when open, so only ever
	 *  created/cached on GetMenuOwnerController()'s instance regardless of which local player's input
	 *  opened it - see GetMenuOwnerController */
	UPROPERTY(Transient)
	TObjectPtr<UCPInGamePauseWidget> PauseWidgetInstance;

	/** Same one-shared-instance rule as PauseWidgetInstance - see GetMenuOwnerController */
	UPROPERTY(Transient)
	TObjectPtr<UCPEndingWidget> EndingWidgetInstance;

	/** True MenuNavigateAction's axis was already processed as one selection-move for the current stick
	 *  push, so it doesn't repeat every frame while held. Reset once the axis returns to the dead zone.
	 *  Kept per-instance (this controller's own device), not on the menu owner, since each local player's
	 *  stick input is received on their own controller instance */
	bool bHasProcessedMenuNavigateThisHold = false;

	/** MenuNavigateAction axis values below this are treated as neutral (stick drift/noise) */
	UPROPERTY(EditAnywhere, Category="Input|Pause", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MenuNavigateDeadZone = 0.5f;

public:

	/** Constructor */
	ACPTopDownPlayerController();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Bound to ToggleDebugWidgetAction. Creates DebugWidgetInstance on first use, then shows/hides it.
	 *  Works regardless of which player/input device triggers it - only no-ops if DebugWidgetClass is unset */
	void ToggleDebugWidget(const FInputActionValue& Value);

	/** Bound to PauseAction. Toggles the world's paused state and GetMenuOwnerController()'s
	 *  InGamePause widget, regardless of which local player's device triggered it. No-ops while the
	 *  Ending widget is showing (that screen only offers End Game/Return to Title, not resume) */
	void TogglePauseMenu(const FInputActionValue& Value);

	/** Bound to MenuNavigateAction. Forwards the (debounced) stick direction to GetActiveMenuWidget's
	 *  MoveSelection, if a menu is currently open */
	void HandleMenuNavigate(const FInputActionValue& Value);

	/** Bound to MenuConfirmAction. Calls GetActiveMenuWidget's ConfirmSelection, if a menu is currently open */
	void HandleMenuConfirm(const FInputActionValue& Value);

	/** Creates (first time only) and shows/hides this instance's PauseWidgetInstance. Only ever called on
	 *  GetMenuOwnerController()'s instance - see the class comment above PauseWidgetInstance */
	void SetPauseMenuVisible(bool bVisible);

	/** Creates (first time only) and shows this instance's EndingWidgetInstance with the given result.
	 *  Only ever called on GetMenuOwnerController()'s instance */
	void SetEndingMenuVisible(bool bIsClear);

	/** Both PauseWidgetInstance and EndingWidgetInstance cover the entire screen (both local players'
	 *  split-screen halves) when shown, so there is no need for one instance per local player the way
	 *  ACPRoulette keeps one widget per player for its own (per-player-positioned) result UI - a single
	 *  shared instance is enough. Regardless of which local PlayerController's input triggers a
	 *  pause/ending action, that instance is always the world's first local PlayerController, so every
	 *  caller resolves it through this function instead of using "this" directly */
	ACPTopDownPlayerController* GetMenuOwnerController() const;

	/** Returns GetMenuOwnerController()'s currently visible menu widget (Ending takes priority over
	 *  Pause, since Ending is only ever shown once Pause has been replaced by it) or nullptr if neither
	 *  is currently visible - used by HandleMenuNavigate/HandleMenuConfirm to find their target */
	UCPInGamePauseWidget* GetActiveMenuWidget() const;

public:

	/** Call when a win/lose condition is met (level design, a test Pawn's debug key, etc.) - pauses the
	 *  game and shows EndingWidgetClass's Clear (bIsClear=true) or Lose (bIsClear=false) result. Safe to
	 *  call on any local player's controller; always ends up on GetMenuOwnerController()'s single shared
	 *  instance regardless of which one is called */
	UFUNCTION(BlueprintCallable, Category="UI|Ending")
	void ShowEndingResult(bool bIsClear);

	/** Returns the world location under the mouse cursor, used to aim the basic attack */
	UFUNCTION(BlueprintCallable, Category="Input")
	bool GetCursorWorldLocation(FVector& OutWorldLocation) const;

	/** Returns true if this controller's platform user currently owns the default input device
	 *  (keyboard/mouse) - false for a player mapped to a gamepad instead (see ACPGameMode::BeginPlay).
	 *  ACPPlayerCharacter uses this to decide whether to aim with the cursor or the movement direction */
	UFUNCTION(BlueprintPure, Category="Input")
	bool IsUsingKeyboardAndMouse() const;
};

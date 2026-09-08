// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CPCoinPusherCaptureTestPlayerController.generated.h"

class UCPCoinPusherCaptureWidget;

/**
 *  PlayerController for the CoinPusher SceneCaptureComponent2D PIP test level. UCPCoinPusherViewportClient
 *  (project-wide, see DefaultEngine.ini) shrinks this controller's local player camera to the right side of
 *  the screen; this controller is what actually creates the UCPCoinPusherCaptureWidget that fills the left
 *  strip and feeds it the capture source's render target. It also points the possessed
 *  ACPCoinPusherCaptureTestPawn (see ACPCoinPusherCaptureTestGameMode) at the capture target so there's
 *  something recognizable to look at on the Player side, and shows the mouse cursor for convenience.
 *
 *  Defaults CaptureWidgetClass to UCPCoinPusherCaptureWidget itself (a pure-C++ widget, no WBP needed) so
 *  this works out of the box - override it in a Blueprint child only if a custom-styled WBP is wanted later.
 */
UCLASS()
class CP_API ACPCoinPusherCaptureTestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ACPCoinPusherCaptureTestPlayerController();

protected:

	/** Widget class (inheriting UCPCoinPusherCaptureWidget) shown full-screen, anchored to the left strip
	 *  that UCPCoinPusherViewportClient leaves free of the player's own camera viewport. Defaults to
	 *  UCPCoinPusherCaptureWidget itself - see constructor */
	UPROPERTY(EditDefaultsOnly, Category="CoinPusher Picture-in-Picture")
	TSubclassOf<UCPCoinPusherCaptureWidget> CaptureWidgetClass;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Deferred one tick past BeginPlay so every level actor (the CoinPusher/test actor included) has
	 *  already run its own BeginPlay and created its capture render target, regardless of actor BeginPlay
	 *  order. Creates CaptureWidgetClass, adds it full-screen, and feeds it the capture target's render
	 *  target */
	void SetupCaptureWidget();
};

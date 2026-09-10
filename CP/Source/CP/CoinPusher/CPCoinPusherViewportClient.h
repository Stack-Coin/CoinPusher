// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "CPCoinPusherViewportClient.generated.h"

/**
 *  Project-wide GameViewportClient (see DefaultEngine.ini, [/Script/Engine.Engine] GameViewportClientClassName)
 *  that shrinks the first local player's own camera viewport to the right (1 - CaptureWidthRatio) of the
 *  screen, leaving the left CaptureWidthRatio (30% by default) free. A UCPCoinPusherCaptureWidget is what
 *  actually fills that left strip (added full-screen via AddToViewport, with its Image anchored to just that
 *  region) - shrinking the Player's own viewport here means the two never render into, or fight over, the
 *  same pixels, unlike drawing an overlay on top of a fullscreen player view.
 */
UCLASS()
class CP_API UCPCoinPusherViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

protected:

	/** Fraction (0-1) of the screen width, from the left edge, reserved for the CoinPusher capture PIP -
	 *  the first local player's viewport starts right after it and fills the rest */
	UPROPERTY(EditDefaultsOnly, Config, Category="CoinPusher Picture-in-Picture", meta = (ClampMin = 0, ClampMax = 1))
	float CaptureWidthRatio = 0.3f;

public:

	/** Runs the normal engine layout first (Super), then overrides the first local player's Origin/Size to
	 *  confine its camera viewport to the right (1 - CaptureWidthRatio) of the screen */
	virtual void LayoutPlayers() override;
};

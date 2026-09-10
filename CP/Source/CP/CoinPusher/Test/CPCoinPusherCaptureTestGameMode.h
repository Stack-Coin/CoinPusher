// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CPCoinPusherCaptureTestGameMode.generated.h"

/**
 *  Minimal GameMode for testing the CoinPusher SceneCaptureComponent2D Picture-in-Picture system in
 *  isolation (see CPCoinPusherCaptureTestActor/CPCoinPusherCaptureTestPlayerController). Uses
 *  ACPCoinPusherCaptureTestPawn (a free-flying pawn with its own camera, no project assets required) so the
 *  remaining 70% of the screen not covered by the PIP overlay always shows a normal, unobstructed Player view.
 */
UCLASS()
class CP_API ACPCoinPusherCaptureTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACPCoinPusherCaptureTestGameMode();
};

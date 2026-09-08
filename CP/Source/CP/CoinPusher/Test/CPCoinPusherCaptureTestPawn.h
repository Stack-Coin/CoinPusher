// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "CPCoinPusherCaptureTestPawn.generated.h"

/**
 *  Player pawn used by the CoinPusher SceneCaptureComponent2D PIP test level (see
 *  ACPCoinPusherCaptureTestGameMode/ACPCoinPusherCaptureTestPlayerController). A thin ADefaultPawn subclass -
 *  free-flying movement and its own camera come from the base class, no project assets required - it only
 *  exists so the test level has its own dedicated pawn class to tweak (fly speed, FOV, etc.) instead of
 *  depending on the engine's ADefaultPawn directly.
 */
UCLASS()
class CP_API ACPCoinPusherCaptureTestPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:

	ACPCoinPusherCaptureTestPawn();
};

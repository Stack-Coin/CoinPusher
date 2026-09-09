// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestGameMode.h"
#include "CPCoinPusherCaptureTestPlayerController.h"
#include "CPCoinPusherItemSpawnTestPawn.h"

ACPCoinPusherItemSpawnTestGameMode::ACPCoinPusherItemSpawnTestGameMode()
{
	DefaultPawnClass = ACPCoinPusherItemSpawnTestPawn::StaticClass();
	PlayerControllerClass = ACPCoinPusherCaptureTestPlayerController::StaticClass();
}

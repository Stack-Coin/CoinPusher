// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherCaptureTestGameMode.h"
#include "CPCoinPusherCaptureTestPlayerController.h"
#include "CPCoinPusherCaptureTestPawn.h"

ACPCoinPusherCaptureTestGameMode::ACPCoinPusherCaptureTestGameMode()
{
	DefaultPawnClass = ACPCoinPusherCaptureTestPawn::StaticClass();
	PlayerControllerClass = ACPCoinPusherCaptureTestPlayerController::StaticClass();
}

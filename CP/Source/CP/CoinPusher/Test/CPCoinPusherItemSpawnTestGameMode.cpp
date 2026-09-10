// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestGameMode.h"
#include "CPCoinPusherCaptureTestPlayerController.h"
#include "CPCoinPusherItemSpawnTestPawn.h"

ACPCoinPusherItemSpawnTestGameMode::ACPCoinPusherItemSpawnTestGameMode()
{
	DefaultPawnClass = ACPCoinPusherItemSpawnTestPawn::StaticClass();
	PlayerControllerClass = ACPCoinPusherCaptureTestPlayerController::StaticClass();
}

void ACPCoinPusherItemSpawnTestGameMode::ReceiveRouletteReward(FName ItemID, int32 SpawnCount)
{
	UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestGameMode] Received roulette reward - ItemID: %s, SpawnCount: %d"), *ItemID.ToString(), SpawnCount);
}

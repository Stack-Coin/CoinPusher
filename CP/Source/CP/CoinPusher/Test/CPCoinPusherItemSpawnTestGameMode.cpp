// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestGameMode.h"
#include "CPCoinPusherCaptureTestPlayerController.h"
#include "CPCoinPusherItemSpawnTestPawn.h"
#include "UObject/Class.h"

ACPCoinPusherItemSpawnTestGameMode::ACPCoinPusherItemSpawnTestGameMode()
{
	DefaultPawnClass = ACPCoinPusherItemSpawnTestPawn::StaticClass();
	PlayerControllerClass = ACPCoinPusherCaptureTestPlayerController::StaticClass();
}

void ACPCoinPusherItemSpawnTestGameMode::ReceiveRouletteReward(FName ItemID, int32 SpawnCount, ECPCoinType CoinType)
{
	UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestGameMode] Received roulette reward - ItemID: %s, SpawnCount: %d, CoinType: %s"),
		*ItemID.ToString(), SpawnCount, *UEnum::GetValueAsString(CoinType));
}

void ACPCoinPusherItemSpawnTestGameMode::ReceiveDroppedItem(FName ItemID, int32 Count, ECPCoinType CoinType)
{
	UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestGameMode] Received dropped item - ItemID: %s, Count: %d, CoinType: %s"),
		*ItemID.ToString(), Count, *UEnum::GetValueAsString(CoinType));
}

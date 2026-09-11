// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestGameMode.h"
#include "CPCoinPusherItemSpawnTestPlayerController.h"
#include "CPCoinPusherItemSpawnTestPawn.h"
#include "Log/CPLogCategories.h"
#include "UObject/Class.h"

ACPCoinPusherItemSpawnTestGameMode::ACPCoinPusherItemSpawnTestGameMode()
{
	DefaultPawnClass = ACPCoinPusherItemSpawnTestPawn::StaticClass();
	PlayerControllerClass = ACPCoinPusherItemSpawnTestPlayerController::StaticClass();
}

void ACPCoinPusherItemSpawnTestGameMode::ReceiveDroppedItem(FName ItemID, int32 Count, ECPCoinType CoinType)
{
	UE_LOG(LogDropZone, Warning, TEXT("[ACPCoinPusherItemSpawnTestGameMode] Received dropped item - ItemID: %s, Count: %d, CoinType: %s"),
		*ItemID.ToString(), Count, *UEnum::GetValueAsString(CoinType));
}

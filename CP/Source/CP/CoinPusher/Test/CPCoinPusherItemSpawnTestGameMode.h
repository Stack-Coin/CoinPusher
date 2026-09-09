// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CPCoinPusherItemSpawnTestGameMode.generated.h"

/**
 *  Minimal GameMode for testing ACPCoinPusher::ItemSpawn() (see ACPCoinPusherItemSpawnTestPawn). Just sets
 *  DefaultPawnClass - the base APlayerController is enough since the pawn handles its own input.
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACPCoinPusherItemSpawnTestGameMode();
};

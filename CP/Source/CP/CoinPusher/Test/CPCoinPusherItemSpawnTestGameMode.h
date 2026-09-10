// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Roulette/CPRouletteRewardReceiver.h"
#include "CoinPusher/CPDroppedItemReceiver.h"
#include "CPCoinPusherItemSpawnTestGameMode.generated.h"

/**
 *  Minimal GameMode for testing ACPCoinPusher::ItemSpawn() (see ACPCoinPusherItemSpawnTestPawn),
 *  ACPRoulette's GameMode reward path, and ACPDropZone's dropped-item notification (see
 *  ACPCoinPusherItemSpawnTestPawn's R key binding). Just sets DefaultPawnClass - the base
 *  APlayerController is enough since the pawn handles its own input.
 *  Implements ICPRouletteRewardReceiver and ICPDroppedItemReceiver directly (instead of inheriting the
 *  full ACPGameMode) so a Roulette slot with RewardTarget = GameMode, or an item/coin dropped into a
 *  DropZone, can be verified in isolation, without any of ACPGameMode's local-multiplayer/team-resource
 *  setup running.
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestGameMode : public AGameModeBase, public ICPRouletteRewardReceiver, public ICPDroppedItemReceiver
{
	GENERATED_BODY()

public:

	ACPCoinPusherItemSpawnTestGameMode();

	/** Confirms the roulette's GameMode reward path reached this GameMode by logging what it received */
	virtual void ReceiveRouletteReward(FName ItemID, int32 SpawnCount, ECPCoinType CoinType) override;

	/** Confirms ACPDropZone's dropped-item notification reached this GameMode by logging what it received */
	virtual void ReceiveDroppedItem(FName ItemID, int32 Count, ECPCoinType CoinType) override;
};

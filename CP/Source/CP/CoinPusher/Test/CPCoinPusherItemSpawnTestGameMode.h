// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoinPusher/CPDroppedItemReceiver.h"
#include "CPCoinPusherItemSpawnTestGameMode.generated.h"

/**
 *  Minimal GameMode for testing ACPCoinPusher::ItemSpawn() (see ACPCoinPusherItemSpawnTestPawn),
 *  ACPDropZone's dropped-item notification, the InGamePause/Ending UI (Z/X keys on the pawn), and the
 *  "InGameUI" HUD (H/J/K/L/G/V/C keys on the pawn - see ACPCoinPusherItemSpawnTestPawn).
 *  PlayerControllerClass is ACPCoinPusherItemSpawnTestPlayerController (an ACPTopDownPlayerController
 *  subclass) so the Pause/Ending input actions, InGamePauseWidgetClass/EndingWidgetClass, and
 *  InGameWidgetClass are available - all need to be filled in on a BP subclass of that controller,
 *  since Input Actions/WBP classes can't be hardcoded in C++ (see
 *  ACPCoinPusherItemSpawnTestPlayerController's comment).
 *  Implements ICPDroppedItemReceiver directly (instead of inheriting the full ACPGameMode) so an
 *  item/coin dropped into a DropZone can be verified in isolation, without any of ACPGameMode's
 *  local-multiplayer/team-resource setup running.
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestGameMode : public AGameModeBase, public ICPDroppedItemReceiver
{
	GENERATED_BODY()

public:

	ACPCoinPusherItemSpawnTestGameMode();

	/** Confirms ACPDropZone's dropped-item notification reached this GameMode by logging what it received */
	virtual void ReceiveDroppedItem(FName ItemID, int32 Count, ECPCoinType CoinType) override;
};

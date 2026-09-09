// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "CPCoinPusherItemSpawnTestPawn.generated.h"

class ACPCoinPusher;
class UInputComponent;

/**
 *  Pawn used to test ACPCoinPusher::ItemSpawn() in isolation. A thin ADefaultPawn subclass - its own camera
 *  and free-fly WASD/mouse movement come from the base class, no project assets required. Pressing Space
 *  calls TargetCoinPusher->ItemSpawn(CoinItemID, 1) to spawn one coin.
 *
 *  TargetCoinPusher can be assigned directly (e.g. if this pawn is placed in the level with
 *  AutoPossessPlayer set, so it shows up as a level instance with an editable Details panel); if left unset,
 *  BeginPlay falls back to finding any ACPCoinPusher placed in the level - so this also works out of the box
 *  when used as a GameMode's DefaultPawnClass (see ACPCoinPusherItemSpawnTestGameMode).
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestPawn : public ADefaultPawn
{
	GENERATED_BODY()

protected:

	/** CoinPusher whose ItemSpawn() is called on Space. Assign directly here, or leave unset to
	 *  auto-find any ACPCoinPusher placed in the level (see BeginPlay) */
	UPROPERTY(EditInstanceOnly, Category="CoinPusher Test")
	TObjectPtr<ACPCoinPusher> TargetCoinPusher;

	/** ItemID passed to ItemSpawn() - must match a key registered in whichever UCPItemRegistry the level's
	 *  ceiling Dispensers use for coins */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName CoinItemID = TEXT("100");

	/** Falls back to finding a level-placed ACPCoinPusher if TargetCoinPusher was left unset */
	virtual void BeginPlay() override;

	/** Adds the Space bar binding on top of ADefaultPawn's own free-fly movement bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Bound to Space - spawns one coin via TargetCoinPusher->ItemSpawn() */
	void HandleSpawnCoinInput();
};

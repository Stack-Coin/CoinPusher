// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "CPCoinPusherItemSpawnTestPawn.generated.h"

class ACPCoinPusher;
class ACPRoulette;
class UInputComponent;

/**
 *  Pawn used to test ACPCoinPusher::ItemSpawn(), ACPCoinPusher::GetPassiveCoinConvertArea()->ConvertActive()/
 *  HPConvertActive(), ACPCoinPusher::ActiveWaveThrow(), ACPCoinPusher::SpawnBigCoin(), and ACPRoulette::Roll()
 *  in isolation.
 *  A thin ADefaultPawn subclass - its own camera and free-fly WASD/mouse movement come from the base class,
 *  no project assets required.
 *  Space spawns one coin via TargetCoinPusher->ItemSpawn(CoinItemID, 1);
 *  P converts PassiveConvertCount coins to Passive via ConvertActive();
 *  O converts HPConvertCount Normal coins to HP via HPConvertActive();
 *  M triggers TargetCoinPusher->ActiveWaveThrow();
 *  I drops one Big-type coin via TargetCoinPusher->SpawnBigCoin();
 *  1/2/3/4/5/6 trigger TargetCoinPusher->GetCoinTowerSpawner()->SpawnTower(N) with N = 5/10/15/20/25/30 floors;
 *  R triggers TargetRoulette->Roll() - each slot's RewardTarget (CoinPusher/GameMode) determines whether the
 *  result reaches TargetCoinPusher->ItemSpawn() or ACPCoinPusherItemSpawnTestGameMode::ReceiveRouletteReward().
 *
 *  This pawn only knows about TargetCoinPusher/TargetRoulette - the convert area is owned by the CoinPusher
 *  itself (as a ChildActorComponent, see ACPCoinPusher::PassiveCoinConvertAreaComponent) and reached through
 *  it. Both can be assigned directly (e.g. if this pawn is placed in the level with AutoPossessPlayer set, so
 *  it shows up as a level instance with an editable Details panel); if left unset, BeginPlay falls back to
 *  finding any ACPCoinPusher/ACPRoulette placed in the level - so this also works out of the box when used as
 *  a GameMode's DefaultPawnClass (see ACPCoinPusherItemSpawnTestGameMode).
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestPawn : public ADefaultPawn
{
	GENERATED_BODY()

protected:

	/** CoinPusher targeted by every key binding below. Assign directly here, or leave unset to auto-find any
	 *  ACPCoinPusher placed in the level (see BeginPlay) */
	UPROPERTY(EditInstanceOnly, Category="CoinPusher Test")
	TObjectPtr<ACPCoinPusher> TargetCoinPusher;

	/** Roulette triggered by the R key. Assign directly here, or leave unset to auto-find any ACPRoulette
	 *  placed in the level (see BeginPlay) */
	UPROPERTY(EditInstanceOnly, Category="CoinPusher Test")
	TObjectPtr<ACPRoulette> TargetRoulette;

	/** ItemID passed to ItemSpawn() - must match a key registered in whichever UCPItemRegistry the level's
	 *  ceiling Dispensers use for coins */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName CoinItemID = TEXT("100");

	/** Num passed to ConvertActive() on P */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 PassiveConvertCount = 5;

	/** Num passed to HPConvertActive() on O */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 HPConvertCount = 5;

	/** Falls back to finding a level-placed ACPCoinPusher if TargetCoinPusher was left unset */
	virtual void BeginPlay() override;

	/** Adds the Space/P/O/M/I/1-6 bindings on top of ADefaultPawn's own free-fly movement bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Bound to Space - spawns one coin via TargetCoinPusher->ItemSpawn() */
	void HandleSpawnCoinInput();

	/** Bound to R - triggers TargetRoulette->Roll() */
	void HandleRollRouletteInput();

	/** Bound to P - converts PassiveConvertCount coins via
	 *  TargetCoinPusher->GetPassiveCoinConvertArea()->ConvertActive() */
	void HandleConvertActiveInput();

	/** Bound to O - converts HPConvertCount Normal coins to HP via
	 *  TargetCoinPusher->GetPassiveCoinConvertArea()->HPConvertActive() */
	void HandleHPConvertActiveInput();

	/** Bound to M - triggers TargetCoinPusher->ActiveWaveThrow() */
	void HandleActiveWaveThrowInput();

	/** Bound to I - drops one Big-type coin via TargetCoinPusher->SpawnBigCoin() */
	void HandleSpawnBigCoinInput();

	/** Bound to 1/2/3/4/5/6 - each calls SpawnCoinTower() with a different fixed floor count (5/10/15/20/25/30) */
	void HandleSpawnCoinTower5Input();
	void HandleSpawnCoinTower10Input();
	void HandleSpawnCoinTower15Input();
	void HandleSpawnCoinTower20Input();
	void HandleSpawnCoinTower25Input();
	void HandleSpawnCoinTower30Input();

	/** Shared by the 1-6 handlers above - calls TargetCoinPusher->GetCoinTowerSpawner()->SpawnTower(FloorCount) */
	void SpawnCoinTower(int32 FloorCount);
};

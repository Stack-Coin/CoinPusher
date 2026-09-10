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
 *  HPConvertActive(), ACPCoinPusher::GetMonsterCoinConvertArea()->MonsterConvertActive(),
 *  ACPCoinPusher::ActiveWaveThrow(), ACPCoinPusher::SpawnBigCoin(), ACPCoinPusher::SpawnMonsterCoin(), and
 *  ACPRoulette::Roll() in isolation.
 *  A thin ADefaultPawn subclass - its own camera and free-fly WASD/mouse movement come from the base class,
 *  no project assets required.
 *  Space spawns one coin via TargetCoinPusher->ItemSpawn(CoinItemID, 1);
 *  P converts PassiveConvertCount coins to Passive via ConvertActive();
 *  O converts HPConvertCount Normal coins to HP via HPConvertActive();
 *  M converts MonsterConvertCount Normal coins to Monster via MonsterConvertActive();
 *  N triggers TargetCoinPusher->ActiveWaveThrow();
 *  I drops one Big-type coin via TargetCoinPusher->SpawnBigCoin();
 *  U drops MonsterCoinSpawnCount Monster-type coins via TargetCoinPusher->SpawnMonsterCoin();
 *  1/2/3/4/5/6 trigger TargetCoinPusher->GetCoinTowerSpawner()->SpawnTower(N) with N = 5/10/15/20/25/30 floors;
 *  R triggers TargetRoulette->Roll() - the winning item is broadcast via ACPRoulette::OnPickedUp, which
 *  whichever ACPCoinPusher has this Roulette assigned as its LinkedRoulette is subscribed to, so the
 *  result reaches that CoinPusher's ItemSpawn() automatically (see ACPCoinPusher::LinkedRoulette);
 *  Z/X test the Ending UI via the possessing ACPTopDownPlayerController::ShowEndingResult() - Z shows the
 *  Clear result, X shows the Lose result (see ACPCoinPusherItemSpawnTestPlayerController). The InGamePause
 *  menu itself isn't bound here - it's opened/closed by ACPTopDownPlayerController::PauseAction
 *  (gamepad Menu button / keyboard Escape, mapped in the controller's Input Mapping Context).
 *
 *  This pawn only knows about TargetCoinPusher/TargetRoulette - the convert areas are owned by the CoinPusher
 *  itself (as ChildActorComponents, see ACPCoinPusher::PassiveCoinConvertAreaComponent/
 *  MonsterCoinConvertAreaComponent) and reached through it. Both can be assigned directly (e.g. if this pawn
 *  is placed in the level with AutoPossessPlayer set, so it shows up as a level instance with an editable
 *  Details panel); if left unset, BeginPlay falls back to finding any ACPCoinPusher/ACPRoulette placed in the
 *  level - so this also works out of the box when used as a GameMode's DefaultPawnClass (see
 *  ACPCoinPusherItemSpawnTestGameMode).
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

	/** ItemID passed to ItemSpawn() - must match a row (RowName) in whichever ItemDataTable the level's
	 *  ceiling Dispensers use for coins (default matches the "Normal Coin" row, ID "1C") */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName CoinItemID = TEXT("1C");

	/** Num passed to ConvertActive() on P */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 PassiveConvertCount = 5;

	/** Num passed to HPConvertActive() on O */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 HPConvertCount = 5;

	/** Num passed to MonsterConvertActive() on M */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 MonsterConvertCount = 5;

	/** Num passed to SpawnMonsterCoin() on U */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 MonsterCoinSpawnCount = 10;

	/** Falls back to finding a level-placed ACPCoinPusher if TargetCoinPusher was left unset */
	virtual void BeginPlay() override;

	/** Adds the Space/P/O/M/N/I/U/1-6 bindings on top of ADefaultPawn's own free-fly movement bindings */
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

	/** Bound to M - converts MonsterConvertCount Normal coins to Monster via
	 *  TargetCoinPusher->GetMonsterCoinConvertArea()->MonsterConvertActive() */
	void HandleMonsterConvertActiveInput();

	/** Bound to N - triggers TargetCoinPusher->ActiveWaveThrow() (moved off M to make room for
	 *  HandleMonsterConvertActiveInput) */
	void HandleActiveWaveThrowInput();

	/** Bound to I - drops one Big-type coin via TargetCoinPusher->SpawnBigCoin() */
	void HandleSpawnBigCoinInput();

	/** Bound to U - drops MonsterCoinSpawnCount Monster-type coins via TargetCoinPusher->SpawnMonsterCoin() */
	void HandleSpawnMonsterCoinInput();

	/** Bound to 1/2/3/4/5/6 - each calls SpawnCoinTower() with a different fixed floor count (5/10/15/20/25/30) */
	void HandleSpawnCoinTower5Input();
	void HandleSpawnCoinTower10Input();
	void HandleSpawnCoinTower15Input();
	void HandleSpawnCoinTower20Input();
	void HandleSpawnCoinTower25Input();
	void HandleSpawnCoinTower30Input();

	/** Shared by the 1-6 handlers above - calls TargetCoinPusher->GetCoinTowerSpawner()->SpawnTower(FloorCount) */
	void SpawnCoinTower(int32 FloorCount);

	/** Bound to Z - calls ShowEndingResult(true) (Clear) on the possessing ACPTopDownPlayerController, if any */
	void HandleShowClearEndingInput();

	/** Bound to X - calls ShowEndingResult(false) (Lose) on the possessing ACPTopDownPlayerController, if any */
	void HandleShowLoseEndingInput();
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "CPCoinPusherItemSpawnTestPawn.generated.h"

class ACPCoinPusher;
class ACPRoulette;
class UInputComponent;

/**
 *  Pawn used to test ACPCoinPusher::ItemSpawn(), ACPCoinPusher::ConvertActive()/HPConvertActive()/
 *  MonsterConvertActive(), ACPCoinPusher::ActiveWaveThrow(), ACPCoinPusher::SpawnBigCoin()/
 *  SpawnMonsterCoin()/SpawnTower(), and ACPRoulette::Roll() in isolation.
 *  A thin ADefaultPawn subclass - its own camera and free-fly WASD/mouse movement come from the base class,
 *  no project assets required.
 *  Space spawns one coin via TargetCoinPusher->ItemSpawn(CoinItemID, 1);
 *  P converts PassiveConvertCount coins to Passive via TargetCoinPusher->ConvertActive(PassiveConvertItemID, ...);
 *  O converts HPConvertCount Normal coins to HP via TargetCoinPusher->HPConvertActive(HPConvertItemID, ...);
 *  M converts MonsterConvertCount Normal coins to Monster via TargetCoinPusher->MonsterConvertActive(MonsterConvertItemID, ...);
 *  N triggers TargetCoinPusher->ActiveWaveThrow();
 *  I drops one Big-type coin via TargetCoinPusher->SpawnBigCoin(BigCoinItemID);
 *  U drops MonsterCoinSpawnCount Monster-type coins via TargetCoinPusher->SpawnMonsterCoin(MonsterCoinItemID, ...);
 *  1/2/3/4/5/6 trigger TargetCoinPusher->SpawnTower(CoinTowerItemID, N) with N = 5/10/15/20/25/30 floors;
 *  R triggers TargetRoulette->Roll() - the winning item is broadcast via ACPRoulette::OnPickedUp, which
 *  whichever ACPCoinPusher has this Roulette assigned as its LinkedRoulette is subscribed to, so the
 *  result reaches that CoinPusher's HandleRoulettePickedUp()/ItemSpawn() automatically (see
 *  ACPCoinPusher::LinkedRoulette);
 *  Z/X test the Ending UI via the possessing ACPTopDownPlayerController::ShowEndingResult() - Z shows the
 *  Clear result, X shows the Lose result (see ACPCoinPusherItemSpawnTestPlayerController). The InGamePause
 *  menu itself isn't bound here - it's opened/closed by ACPTopDownPlayerController::PauseAction
 *  (gamepad Menu button / keyboard Escape, mapped in the controller's Input Mapping Context).
 *
 *  Every key binding above goes through a wrapper on ACPCoinPusher itself, not through
 *  GetPassiveCoinConvertArea()/GetMonsterCoinConvertArea()/GetCoinTowerSpawner() directly - each wrapper
 *  looks up the passed ItemID in ACPCoinPusher::ItemDataTable and only forwards to the real implementation
 *  if the row's CoinType matches what that wrapper expects (Big/Monster/Passive/HP/Monster/CoinTower
 *  respectively), so this pawn only knows about TargetCoinPusher/TargetRoulette.
 *  Both can be assigned directly (e.g. if this pawn is placed in the level with AutoPossessPlayer set, so
 *  it shows up as a level instance with an editable Details panel); if left unset, BeginPlay falls back to
 *  finding any ACPCoinPusher/ACPRoulette placed in the level - so this also works out of the box when used
 *  as a GameMode's DefaultPawnClass (see ACPCoinPusherItemSpawnTestGameMode).
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

	/** ItemID passed to TargetCoinPusher->ConvertActive() on P - must match a row (RowName) in
	 *  TargetCoinPusher->ItemDataTable whose CoinType is Passive, or the wrapper does nothing */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName PassiveConvertItemID = TEXT("2C");

	/** Num passed to TargetCoinPusher->ConvertActive() on P */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 PassiveConvertCount = 5;

	/** ItemID passed to TargetCoinPusher->HPConvertActive() on O - must match a row (RowName) in
	 *  TargetCoinPusher->ItemDataTable whose CoinType is HP, or the wrapper does nothing */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName HPConvertItemID = TEXT("3C");

	/** Num passed to TargetCoinPusher->HPConvertActive() on O */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 HPConvertCount = 5;

	/** ItemID passed to TargetCoinPusher->MonsterConvertActive() on M - must match a row (RowName) in
	 *  TargetCoinPusher->ItemDataTable whose CoinType is Monster, or the wrapper does nothing */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName MonsterConvertItemID = TEXT("5C");

	/** Num passed to TargetCoinPusher->MonsterConvertActive() on M */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 MonsterConvertCount = 5;

	/** ItemID passed to TargetCoinPusher->SpawnBigCoin() on I - must match a row (RowName) in
	 *  TargetCoinPusher->ItemDataTable whose CoinType is Big, or the wrapper does nothing */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName BigCoinItemID = TEXT("4C");

	/** ItemID passed to TargetCoinPusher->SpawnMonsterCoin() on U - must match a row (RowName) in
	 *  TargetCoinPusher->ItemDataTable whose CoinType is Monster, or the wrapper does nothing */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName MonsterCoinItemID = TEXT("5C");

	/** Num passed to TargetCoinPusher->SpawnMonsterCoin() on U */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test", meta = (ClampMin = 1))
	int32 MonsterCoinSpawnCount = 10;

	/** ItemID passed to TargetCoinPusher->SpawnTower() on 1-6 - must match a row (RowName) in
	 *  TargetCoinPusher->ItemDataTable whose CoinType is CoinTower, or the wrapper does nothing */
	UPROPERTY(EditAnywhere, Category="CoinPusher Test")
	FName CoinTowerItemID = TEXT("6C");

	/** Falls back to finding a level-placed ACPCoinPusher if TargetCoinPusher was left unset */
	virtual void BeginPlay() override;

	/** Adds the Space/P/O/M/N/I/U/1-6 bindings on top of ADefaultPawn's own free-fly movement bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Bound to Space - spawns one coin via TargetCoinPusher->ItemSpawn() */
	void HandleSpawnCoinInput();

	/** Bound to R - triggers TargetRoulette->Roll() */
	void HandleRollRouletteInput();

	/** Bound to P - converts PassiveConvertCount coins via
	 *  TargetCoinPusher->ConvertActive(PassiveConvertItemID, PassiveConvertCount) */
	void HandleConvertActiveInput();

	/** Bound to O - converts HPConvertCount Normal coins to HP via
	 *  TargetCoinPusher->HPConvertActive(HPConvertItemID, HPConvertCount) */
	void HandleHPConvertActiveInput();

	/** Bound to M - converts MonsterConvertCount Normal coins to Monster via
	 *  TargetCoinPusher->MonsterConvertActive(MonsterConvertItemID, MonsterConvertCount) */
	void HandleMonsterConvertActiveInput();

	/** Bound to N - triggers TargetCoinPusher->ActiveWaveThrow() (moved off M to make room for
	 *  HandleMonsterConvertActiveInput) */
	void HandleActiveWaveThrowInput();

	/** Bound to I - drops one Big-type coin via TargetCoinPusher->SpawnBigCoin(BigCoinItemID) */
	void HandleSpawnBigCoinInput();

	/** Bound to U - drops MonsterCoinSpawnCount Monster-type coins via
	 *  TargetCoinPusher->SpawnMonsterCoin(MonsterCoinItemID, MonsterCoinSpawnCount) */
	void HandleSpawnMonsterCoinInput();

	/** Bound to 1/2/3/4/5/6 - each calls SpawnCoinTower() with a different fixed floor count (5/10/15/20/25/30) */
	void HandleSpawnCoinTower5Input();
	void HandleSpawnCoinTower10Input();
	void HandleSpawnCoinTower15Input();
	void HandleSpawnCoinTower20Input();
	void HandleSpawnCoinTower25Input();
	void HandleSpawnCoinTower30Input();

	/** Shared by the 1-6 handlers above - calls TargetCoinPusher->SpawnTower(CoinTowerItemID, FloorCount) */
	void SpawnCoinTower(int32 FloorCount);

	/** Bound to Z - calls ShowEndingResult(true) (Clear) on the possessing ACPTopDownPlayerController, if any */
	void HandleShowClearEndingInput();

	/** Bound to X - calls ShowEndingResult(false) (Lose) on the possessing ACPTopDownPlayerController, if any */
	void HandleShowLoseEndingInput();
};

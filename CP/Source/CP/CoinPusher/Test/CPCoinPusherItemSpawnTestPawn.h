// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "CPCoinPusherItemSpawnTestPawn.generated.h"

class ACPCoinPusher;
class ACPRoulette;
class UInputComponent;
class UCPInGameWidget;
class UTexture2D;

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
 *  H/J/K/L/G/V/C test the "InGameUI" HUD (UI/CPInGameWidget.h) created by the possessing
 *  controller's ACPTopDownPlayerController::GetInGameWidget() - all of them simply push locally
 *  held fake stat values into that widget, no real gameplay system involved. F2-F9 toggle each
 *  InGameUI sub-component on/off (see HandleTogglePlayerInfoInput's comment for the full list):
 *  H damages the player's test health (PlayerHealth -= StatChangeAmount, resets to PlayerMaxHealth
 *  once it reaches 0) via InGameUI->UpdatePlayerHealth();
 *  J gains player test exp (wraps back to 0 once it reaches PlayerMaxExp, simulating a level up) via
 *  InGameUI->UpdatePlayerExp();
 *  K/L do the same for the boss's test health/exp via UpdateBossHealth()/UpdateBossExp();
 *  G gains one test ticket via InGameUI->UpdateTicketCount();
 *  V raises the test combo count by one and refills the combo gauge to ComboGaugeMax via
 *  InGameUI->SetComboCount()/UpdateComboGauge() (simulating a successful combo hit);
 *  C breaks the test combo back to 0/empty via the same two functions (simulating a combo miss).
 *  PlayerName/BossName/PlayerPortrait/BossPortrait are pushed once in BeginPlay instead of being
 *  bound to a key, since they aren't the kind of value that needs repeated testing.
 *
 *  Escape calls ACPTopDownPlayerController::TogglePauseMenu() directly (legacy key, bypassing
 *  Enhanced Input) so the pause menu can be tested even before a PauseAction/Input Mapping Context is
 *  set up on a BP subclass of the controller - see HandleTogglePauseInput's comment.
 *
 *  F2-F9 toggle each InGameUI sub-component's visibility on/off (flips a locally-tracked bool and
 *  calls the matching UCPInGameWidget::Set*Visible()) - starts matching InGameUI's own defaults
 *  (everything on except BossInfoWidget, which UCPInGameWidget::NativeConstruct hides by default):
 *  F2 PlayerInfoWidget, F3 BossInfoWidget (starts off), F4 BackgroundImage, F5 TicketCountWidget,
 *  F6 InventoryWidget, F7 CoinComboWidget, F8 RouletteWidget, F9 CoinPointUI.
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

	/** Fake current/max health, exp, ticket and combo values driving the InGameUI test (H/J/K/L/G/V/C) -
	 *  no real gameplay stat system involved, see the class comment above */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float PlayerMaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float PlayerHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float PlayerMaxExp = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float PlayerExp = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float BossMaxHealth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float BossHealth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float BossMaxExp = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float BossExp = 0.0f;

	/** H/J/K/L presses step PlayerHealth/PlayerExp/BossHealth/BossExp by this amount */
	UPROPERTY(EditAnywhere, Category="In Game UI Test", meta = (ClampMin = 0))
	float StatChangeAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	int32 TicketCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	int32 ComboCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float ComboGaugeMax = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="In Game UI Test")
	float ComboGaugeCurrent = 0.0f;

	/** BeginPlay에서 한 번만 InGameUI에 반영하는 이름/레벨/초상화 테스트 값 (반복 테스트가 필요 없어
	 *  키 입력 대신 여기서 고정값으로 설정) */
	UPROPERTY(EditAnywhere, Category="In Game UI Test")
	FText PlayerName = FText::FromString(TEXT("Player"));

	UPROPERTY(EditAnywhere, Category="In Game UI Test")
	int32 PlayerLevel = 1;

	UPROPERTY(EditAnywhere, Category="In Game UI Test")
	FText BossName = FText::FromString(TEXT("Boss"));

	UPROPERTY(EditAnywhere, Category="In Game UI Test")
	int32 BossLevel = 1;

	UPROPERTY(EditAnywhere, Category="In Game UI Test")
	TObjectPtr<UTexture2D> PlayerPortrait;

	UPROPERTY(EditAnywhere, Category="In Game UI Test")
	TObjectPtr<UTexture2D> BossPortrait;

	/** Locally-tracked on/off state for the F2-F9 InGameUI component visibility toggles - InGameUI
	 *  itself doesn't expose a getter for current visibility (only Set*Visible setters), so this pawn
	 *  keeps its own bool per component to know which way to flip on each key press. Starts matching
	 *  InGameUI's own defaults (everything on except BossInfoWidget) */
	bool bPlayerInfoVisible = true;
	bool bBossInfoVisible = false;
	bool bBackgroundVisible = true;
	bool bTicketCountVisible = true;
	bool bInventoryVisible = true;
	bool bCoinComboVisible = true;
	bool bRouletteVisible = true;
	bool bCoinPointUIVisible = true;

	/** Falls back to finding a level-placed ACPCoinPusher if TargetCoinPusher was left unset. Also
	 *  schedules PushInitialInGameUIValues() for next tick (see its comment for why not right here) */
	virtual void BeginPlay() override;

	/** Pushes the initial fake stat values (and PlayerName/BossName/-Portrait) into the possessing
	 *  ACPTopDownPlayerController's InGameUI, if any. Deferred to next tick (via a
	 *  SetTimerForNextTick call in BeginPlay, same pattern as
	 *  ACPTopDownPlayerController::SetupCaptureWidget) instead of running directly in BeginPlay,
	 *  since neither this pawn's possession (GetController()) nor the controller's own InGameUI
	 *  creation (its BeginPlay) are guaranteed to have happened yet at this pawn's BeginPlay time */
	void PushInitialInGameUIValues();

	/** Returns the possessing ACPTopDownPlayerController's InGameUI instance, or nullptr if the
	 *  controller isn't that type or has no InGameWidgetClass set */
	UCPInGameWidget* GetInGameWidget() const;

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

	/** Bound to H - steps PlayerHealth down by StatChangeAmount (resetting to PlayerMaxHealth once it
	 *  reaches 0) and pushes it to InGameUI->UpdatePlayerHealth() */
	void HandleDamagePlayerInput();

	/** Bound to J - steps PlayerExp up by StatChangeAmount (wrapping back to 0 once it reaches
	 *  PlayerMaxExp) and pushes it to InGameUI->UpdatePlayerExp() */
	void HandleGainPlayerExpInput();

	/** Bound to K - same as HandleDamagePlayerInput, but for BossHealth/UpdateBossHealth() */
	void HandleDamageBossInput();

	/** Bound to L - same as HandleGainPlayerExpInput, but for BossExp/UpdateBossExp() */
	void HandleGainBossExpInput();

	/** Bound to G - increments TicketCount by one and pushes it to InGameUI->UpdateTicketCount() */
	void HandleGainTicketInput();

	/** Bound to V - increments ComboCount by one, refills ComboGaugeCurrent to ComboGaugeMax, and
	 *  pushes both to InGameUI->SetComboCount()/UpdateComboGauge() (simulating a combo hit) */
	void HandleGainComboInput();

	/** Bound to C - resets ComboCount/ComboGaugeCurrent to 0 and pushes both to InGameUI
	 *  (simulating a combo miss/break) */
	void HandleResetComboInput();

	/** Bound to Escape - calls ACPTopDownPlayerController::TogglePauseMenu() directly on the possessing
	 *  controller, if any. This is a legacy (non-Enhanced-Input) shortcut so the pause menu can be
	 *  tested without setting up a PauseAction/Input Mapping Context on a BP subclass of the controller
	 *  first - InGamePauseWidgetClass (a WBP reference) still needs to be set there regardless, since
	 *  that can't be hardcoded in C++. The real game's PauseAction (gamepad Menu button/keyboard Escape
	 *  via Enhanced Input) keeps working the same way independently of this */
	void HandleTogglePauseInput();

	/** Bound to F2 - flips bPlayerInfoVisible and calls InGameUI->SetPlayerInfoVisible() */
	void HandleTogglePlayerInfoInput();

	/** Bound to F3 - flips bBossInfoVisible and calls InGameUI->SetBossInfoVisible() (starts off,
	 *  matching UCPInGameWidget::NativeConstruct's default) */
	void HandleToggleBossInfoInput();

	/** Bound to F4 - flips bBackgroundVisible and calls InGameUI->SetBackgroundVisible() */
	void HandleToggleBackgroundInput();

	/** Bound to F5 - flips bTicketCountVisible and calls InGameUI->SetTicketCountVisible() */
	void HandleToggleTicketCountInput();

	/** Bound to F6 - flips bInventoryVisible and calls InGameUI->SetInventoryVisible() */
	void HandleToggleInventoryInput();

	/** Bound to F7 - flips bCoinComboVisible and calls InGameUI->SetCoinComboVisible() */
	void HandleToggleCoinComboInput();

	/** Bound to F8 - flips bRouletteVisible and calls InGameUI->SetRouletteVisible() */
	void HandleToggleRouletteInput();

	/** Bound to F9 - flips bCoinPointUIVisible and calls InGameUI->SetCoinPointUIVisible() */
	void HandleToggleCoinPointUIInput();
};

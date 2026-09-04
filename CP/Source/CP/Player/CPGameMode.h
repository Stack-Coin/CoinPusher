// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/CPCoinWallet.h"
#include "Player/CPStatTypes.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "CPGameMode.generated.h"

class ACPPartyCamera;
class ACPPlayerCharacter;
class UCPHealthBarWidget;
class UCPTicketCountWidget;
class UCPCoinCountWidget;
class UCPRadialGaugeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPTeamTicketCountChanged, int32, NewTicketCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPTeamCoinCountChanged, int32, NewCoinCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPTeamLevelUp, int32, NewLevel);

UCLASS(abstract)
class ACPGameMode : public AGameModeBase, public ICPCoinWallet
{
	GENERATED_BODY()

protected:

	//***** �� ���� ����
	UPROPERTY(BlueprintReadOnly, Category="Team")
	int32 TeamTicketCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Team")
	int32 TeamCoinCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Team")
	float TeamExperience = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Team")
	int32 TeamLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team|Ranges")
	FCPStatRange TeamLevelRange = FCPStatRange(1.0f, 99.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team|Ranges")
	FCPStatRange TeamExperienceRange = FCPStatRange(0.0f, 999999.0f);

	// ���� �� �ϴ� ���� �ʿ��� ����ġ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Team|Leveling", meta = (ClampMin = 0))
	float BaseRequiredTeamExperience = 100.0f;

	// ���� ���� �ʿ� ����ġ �߰��ϴ� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Team|Leveling", meta = (ClampMin = 0))
	float RequiredTeamExperiencePerLevel = 0.0f;

	/** How many local players to spawn on game start - fixed at level start, no drop-in join.
	 *  Player 0 (keyboard/mouse) is created automatically as part of regular game init; this only
	 *  creates players 2+ (e.g. Player 1 on the first connected gamepad) */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfLocalPlayers = 2;

	/** Used to assign players to different PlayerStarts (tagged Player0, Player1, ...) in the level */
	int32 CurrentPlayerStartAssignment = 0;

	/** Bound to IPlatformInputDeviceMapper's connection-change event for the lifetime of this GameMode */
	FDelegateHandle InputDeviceConnectionChangeHandle;

	/** Class spawned as the shared single camera (see ToggleCameraMode). Defaults to ACPPartyCamera itself
	 *  if left unset - only needs overriding if a level wants different default zoom/speed values */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|Camera")
	TSubclassOf<ACPPartyCamera> PartyCameraClass;

	/** Blend time used by SetViewTargetWithBlend when swapping between split-screen and the party camera */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|Camera", meta = (ClampMin = 0, Units = "s"))
	float CameraSwapBlendTime = 0.75f;

	/** Spawned lazily the first time single-camera mode is entered, then reused */
	UPROPERTY()
	TObjectPtr<ACPPartyCamera> PartyCamera;

	/** True while every local player is viewing the single shared PartyCamera instead of their own
	 *  split-screen viewport/camera */
	bool bIsSingleCameraMode = false;

	/** Widget Blueprint (inheriting UCPTicketCountWidget) for the team ticket count HUD. Created once in
	 *  BeginPlay and bound directly to OnTeamTicketCountChanged in C++ - no BP graph wiring needed */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPTicketCountWidget> TicketWidgetClass;

	/** Widget Blueprint (inheriting UCPCoinCountWidget) for the team coin count HUD. Created once in
	 *  BeginPlay and bound directly to OnTeamCoinCountChanged in C++ - no BP graph wiring needed */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPCoinCountWidget> CoinWidgetClass;

	/** Widget Blueprint (inheriting UCPHealthBarWidget) for the first local player's (1P) health bar -
	 *  give it a WBP with the bar anchored to the left of the screen */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPHealthBarWidget> Player1HealthBarWidgetClass;

	/** Widget Blueprint (inheriting UCPHealthBarWidget) for the second local player's (2P) health bar -
	 *  give it a WBP with the bar anchored to the right of the screen */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPHealthBarWidget> Player2HealthBarWidgetClass;

	/** Class (inheriting UCPRadialGaugeComponent) dynamically attached to every player pawn in BeginPlay
	 *  to show revive progress. Give it a BP subclass with GaugeWidgetClass (a UCPRadialGaugeWidget WBP)
	 *  and a relative location already set in its Class Defaults - left unset, no gauge is attached */
	UPROPERTY(EditAnywhere, Category="Local Multiplayer|UI")
	TSubclassOf<UCPRadialGaugeComponent> ReviveGaugeComponentClass;

public:

	/** Constructor */
	ACPGameMode();

	/** Creates the additional local players (see NumberOfLocalPlayers), tries to assign the first
	 *  connected gamepad to the second local player right away, and starts watching for one to connect
	 *  later (a gamepad isn't always detected yet by the time BeginPlay runs) */
	virtual void BeginPlay() override;

	/** Stops watching for gamepad connection changes */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Assigns a PlayerStart tagged PlayerN to the Nth player to spawn, in order */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:

	/** Bound to IPlatformInputDeviceMapper::GetOnInputDeviceConnectionChange - catches a gamepad that
	 *  connects/is detected after BeginPlay already ran */
	void HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

	/** If the second local player doesn't already own an input device, assigns it the first currently
	 *  connected non-default device (a gamepad). No-ops if there's no second local player, it already
	 *  has a device, or no gamepad is connected yet */
	void TryAssignGamepadToSecondPlayer();

	/** Creates the team-wide ticket/coin HUD widgets (see TicketWidgetClass/CoinWidgetClass) and binds
	 *  them directly to OnTeamTicketCountChanged/OnTeamCoinCountChanged. Called once from BeginPlay */
	void SetupTeamResourceWidgets();

	/** Creates a health bar widget using HealthBarWidgetClass, adds it to PlayerCharacter's owning
	 *  player's screen, and binds it directly to PlayerCharacter's OnHealthChanged. Called once per
	 *  local player from BeginPlay */
	void SetupPlayerHealthBarWidget(ACPPlayerCharacter* PlayerCharacter, TSubclassOf<UCPHealthBarWidget> HealthBarWidgetClass);

	/** Creates a ReviveGaugeComponentClass instance, attaches it to PlayerCharacter (disabled until a
	 *  revive attempt starts), and hands it to the character via SetReviveGaugeComponent. No-ops if
	 *  ReviveGaugeComponentClass is unset or PlayerCharacter already has one */
	void AttachReviveGaugeToPlayer(ACPPlayerCharacter* PlayerCharacter);

public:

	/** Swaps between per-player split-screen and a single shared camera (ACPPartyCamera) that frames
	 *  every local player and zooms with their spread. Bound to the C key by default (see ACPPlayerCharacter) */
	UFUNCTION(BlueprintCallable, Category="Local Multiplayer")
	void ToggleCameraMode();

	/** Returns true while showing the single shared party camera instead of split-screen */
	UFUNCTION(BlueprintPure, Category="Local Multiplayer")
	bool IsSingleCameraMode() const { return bIsSingleCameraMode; }

	/** Broadcast whenever TeamTicketCount changes */
	UPROPERTY(BlueprintAssignable, Category="Team")
	FOnCPTeamTicketCountChanged OnTeamTicketCountChanged;

	/** Broadcast whenever TeamCoinCount changes */
	UPROPERTY(BlueprintAssignable, Category="Team")
	FOnCPTeamCoinCountChanged OnTeamCoinCountChanged;

	/** Broadcast right after TeamLevel increases by 1 (once per level, even on a multi level up) */
	UPROPERTY(BlueprintAssignable, Category="Team")
	FOnCPTeamLevelUp OnTeamLevelUp;

	/** Adds Amount tickets (e.g. called by ACPDropZone every 10 coins collected) */
	UFUNCTION(BlueprintCallable, Category="Team")
	void AddTeamTickets(int32 Amount = 1);

	/** Returns the current team ticket count */
	UFUNCTION(BlueprintPure, Category="Team")
	int32 GetTeamTicketCount() const { return TeamTicketCount; }

	/** Attempts to spend Amount tickets. Deducts and returns true on success; leaves the count
	 *  unchanged and returns false if there aren't enough tickets */
	UFUNCTION(BlueprintCallable, Category="Team")
	bool TrySpendTeamTicket(int32 Amount = 1);

	/** Adds team experience, handling one or multiple team level ups if enough is accumulated at once */
	UFUNCTION(BlueprintCallable, Category="Team")
	void AddTeamExperience(float Amount);

	/** Returns the current team experience */
	UFUNCTION(BlueprintPure, Category="Team")
	float GetTeamExperience() const { return TeamExperience; }

	/** Returns the current team level */
	UFUNCTION(BlueprintPure, Category="Team")
	int32 GetTeamLevel() const { return TeamLevel; }

	/** Returns the team experience required to go from the current team level to the next */
	UFUNCTION(BlueprintPure, Category="Team")
	float GetRequiredTeamExperience() const;

	// ~begin ICPCoinWallet

	/** Adds Amount to the team's coin balance */
	virtual void AddCoin(int32 Amount) override;

	/** Returns the team's current coin balance */
	virtual int32 GetCoinAmount() const override { return TeamCoinCount; }

	/** Returns true if the team's coin balance is at least Amount */
	virtual bool HasEnoughCoin(int32 Amount) const override { return TeamCoinCount >= Amount; }

	/** Attempts to spend Amount coins from the team's balance. Deducts and returns true on success,
	 *  leaves the balance unchanged and returns false otherwise */
	virtual bool TrySpendCoin(int32 Amount) override;

	// ~end ICPCoinWallet

protected:
	// KohMS
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bIsGameOver = false;

	UFUNCTION(BlueprintPure, Category = "Game")
	bool IsGameOver() const { return bIsGameOver; }

	UFUNCTION()
	void HandleGoddessDead();

protected:
	// KohMS // 화면에 웨이브 진행 상황(웨이브 번호 / 다음 이벤트까지 남은 시간) 표시
	UPROPERTY()
	TObjectPtr<class ACPMonsterSpawner> WaveStatusSourceSpawner;

	UPROPERTY()
	TObjectPtr<class UCPUserWidget_WaveStatus> WaveStatusWidget;

	FTimerHandle WaveStatusUpdateTimer;

	UFUNCTION()
	void UpdateWaveStatusDisplay();
};

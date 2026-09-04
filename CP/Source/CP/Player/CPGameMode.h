// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/CPCoinWallet.h"
#include "Player/CPStatTypes.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "CPGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPTeamTicketCountChanged, int32, NewTicketCount);

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

public:
	/** Broadcast whenever TeamTicketCount changes */
	UPROPERTY(BlueprintAssignable, Category="Team")
	FOnCPTeamTicketCountChanged OnTeamTicketCountChanged;

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

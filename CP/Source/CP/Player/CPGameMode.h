// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/CPStatTypes.h"
#include "CPGameMode.generated.h"

class ACPPlayerCharacter;
class UCPHorizonGuageBarWidget;
class UCPTicketCountWidget;
class UCPCoinCountWidget;
class UCPRadialGaugeComponent;
class UCPInventoryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPTeamLevelUp, int32, NewLevel);

UCLASS(abstract)
class ACPGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	//***** �� ���� ����
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

	/** Widget Blueprint (inheriting UCPTicketCountWidget) for the player's ticket count HUD. Created once in
	 *  BeginPlay and bound directly to the player's OnTicketChanged in C++ - no BP graph wiring needed */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPTicketCountWidget> TicketWidgetClass;

	/** Widget Blueprint (inheriting UCPCoinCountWidget) for the player's score count HUD. Created once in
	 *  BeginPlay and bound directly to the player's OnScoreChanged in C++ - no BP graph wiring needed */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPCoinCountWidget> CoinWidgetClass;

	/** Widget Blueprint (inheriting UCPHorizonGuageBarWidget) for the player's health bar */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPHorizonGuageBarWidget> PlayerHealthBarWidgetClass;

	/** Widget Blueprint (inheriting UCPInventoryWidget) for the player's cross-shaped inventory HUD */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer|UI")
	TSubclassOf<UCPInventoryWidget> InventoryWidgetClass;

	/** Class (inheriting UCPRadialGaugeComponent) dynamically attached to every player pawn in BeginPlay
	 *  to show revive progress. Give it a BP subclass with GaugeWidgetClass (a UCPRadialGaugeWidget WBP)
	 *  and a relative location already set in its Class Defaults - left unset, no gauge is attached */
	UPROPERTY(EditAnywhere, Category="Local Multiplayer|UI")
	TSubclassOf<UCPRadialGaugeComponent> ReviveGaugeComponentClass;

public:

	/** Constructor */
	ACPGameMode();

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Assigns a PlayerStart tagged Player0 to the player, falling back to any PlayerStart in the level */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:

	/** Creates PlayerCharacter's ticket/coin HUD widgets (see TicketWidgetClass/CoinWidgetClass) and binds
	 *  them directly to PlayerCharacter's OnTicketChanged/OnScoreChanged. Called once per local player from BeginPlay */
	void SetupPlayerWalletWidgets(ACPPlayerCharacter* PlayerCharacter);

	/** Creates a health bar widget using HealthBarWidgetClass, adds it to PlayerCharacter's owning
	 *  player's screen, and binds it directly to PlayerCharacter's OnHealthChanged. Called once per
	 *  local player from BeginPlay */
	void SetupPlayerHealthBarWidget(ACPPlayerCharacter* PlayerCharacter, TSubclassOf<UCPHorizonGuageBarWidget> HealthBarWidgetClass);

	/** Creates an inventory widget using InventoryWidgetClass, adds it to PlayerCharacter's owning player's
	 *  screen. Called once per local player from BeginPlay */
	void SetupPlayerInventoryWidget(ACPPlayerCharacter* PlayerCharacter);

	/** Creates a ReviveGaugeComponentClass instance, attaches it to PlayerCharacter (disabled until a
	 *  revive attempt starts), and hands it to the character via SetReviveGaugeComponent. No-ops if
	 *  ReviveGaugeComponentClass is unset or PlayerCharacter already has one */
	void AttachReviveGaugeToPlayer(ACPPlayerCharacter* PlayerCharacter);

public:

	/** Broadcast right after TeamLevel increases by 1 (once per level, even on a multi level up) */
	UPROPERTY(BlueprintAssignable, Category="Team")
	FOnCPTeamLevelUp OnTeamLevelUp;

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
};

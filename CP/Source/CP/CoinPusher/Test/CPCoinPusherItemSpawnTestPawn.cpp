// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestPawn.h"
#include "CoinPusher/CPCoinPusher.h"
#include "Roulette/CPRoulette.h"
#include "Player/CPTopDownPlayerController.h"
#include "UI/CPInGameWidget.h"
#include "Log/CPLogCategories.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void ACPCoinPusherItemSpawnTestPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetCoinPusher)
	{
		TargetCoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass()));
	}

	if (!TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No ACPCoinPusher assigned or found in the level - Space/P/O/M/N/I/U/1-6 will do nothing."));
	}

	if (!TargetRoulette)
	{
		TargetRoulette = Cast<ACPRoulette>(UGameplayStatics::GetActorOfClass(this, ACPRoulette::StaticClass()));
	}

	if (!TargetRoulette)
	{
		UE_LOG(LogRoulette, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No ACPRoulette assigned or found in the level - R will do nothing."));
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &ACPCoinPusherItemSpawnTestPawn::PushInitialInGameUIValues);
}

void ACPCoinPusherItemSpawnTestPawn::PushInitialInGameUIValues()
{
	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetPlayerName(PlayerName);
		InGameWidget->SetPlayerLevel(PlayerLevel);
		InGameWidget->SetPlayerPortrait(PlayerPortrait);
		InGameWidget->UpdatePlayerHealth(PlayerHealth, PlayerMaxHealth);
		InGameWidget->UpdatePlayerExp(PlayerExp, PlayerMaxExp);

		InGameWidget->SetBossName(BossName);
		InGameWidget->SetBossLevel(BossLevel);
		InGameWidget->SetBossPortrait(BossPortrait);
		InGameWidget->UpdateBossHealth(BossHealth, BossMaxHealth);
		InGameWidget->UpdateBossExp(BossExp, BossMaxExp);

		InGameWidget->UpdateTicketCount(TicketCount);
		InGameWidget->SetComboCount(ComboCount);
		InGameWidget->UpdateComboGauge(ComboGaugeCurrent, ComboGaugeMax);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No InGameUI found (controller isn't ACPTopDownPlayerController, or its InGameWidgetClass is unset) - H/J/K/L/G/V/C will do nothing."));
	}
}

UCPInGameWidget* ACPCoinPusherItemSpawnTestPawn::GetInGameWidget() const
{
	ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(GetController());
	return PC ? PC->GetInGameWidget() : nullptr;
}

void ACPCoinPusherItemSpawnTestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleRollRouletteInput);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::O, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleHPConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleMonsterConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleActiveWaveThrowInput);
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnBigCoinInput);
	PlayerInputComponent->BindKey(EKeys::U, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnMonsterCoinInput);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower5Input);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower10Input);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower15Input);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower20Input);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower25Input);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower30Input);
	PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleShowClearEndingInput);
	PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleShowLoseEndingInput);
	PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleDamagePlayerInput);
	PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleGainPlayerExpInput);
	PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleDamageBossInput);
	PlayerInputComponent->BindKey(EKeys::L, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleGainBossExpInput);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleGainTicketInput);
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleGainComboInput);
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleResetComboInput);
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleTogglePauseInput);
	PlayerInputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleTogglePlayerInfoInput);
	PlayerInputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleBossInfoInput);
	PlayerInputComponent->BindKey(EKeys::F4, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleBackgroundInput);
	PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleTicketCountInput);
	PlayerInputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleInventoryInput);
	PlayerInputComponent->BindKey(EKeys::F7, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleCoinComboInput);
	PlayerInputComponent->BindKey(EKeys::F8, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleRouletteInput);
	PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleToggleCoinPointUIInput);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("Coin Spawn"));
		TargetCoinPusher->ItemSpawn(CoinItemID, 1);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleRollRouletteInput()
{
	if (!TargetRoulette)
	{
		return;
	}

	const bool bStarted = TargetRoulette->Roll();
	UE_LOG(LogRoulette, Warning, TEXT("Roll Roulette (started: %s)"), bStarted ? TEXT("true") : TEXT("false"));
}

void ACPCoinPusherItemSpawnTestPawn::HandleConvertActiveInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("Convert Active"));
	TargetCoinPusher->ConvertActive(PassiveConvertItemID, PassiveConvertCount);
}

void ACPCoinPusherItemSpawnTestPawn::HandleHPConvertActiveInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("HP Convert Active"));
	TargetCoinPusher->HPConvertActive(HPConvertItemID, HPConvertCount);
}

void ACPCoinPusherItemSpawnTestPawn::HandleMonsterConvertActiveInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("Monster Convert Active"));
	TargetCoinPusher->MonsterConvertActive(MonsterConvertItemID, MonsterConvertCount);
}

void ACPCoinPusherItemSpawnTestPawn::HandleActiveWaveThrowInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("Active Wave Throw"));
	TargetCoinPusher->ActiveWaveThrow();
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnBigCoinInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("Spawn Big Coin"));
	TargetCoinPusher->SpawnBigCoin(BigCoinItemID);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnMonsterCoinInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("Spawn Monster Coin (%d)"), MonsterCoinSpawnCount);
	TargetCoinPusher->SpawnMonsterCoin(MonsterCoinItemID, MonsterCoinSpawnCount);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower5Input()
{
	SpawnCoinTower(5);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower10Input()
{
	SpawnCoinTower(10);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower15Input()
{
	SpawnCoinTower(15);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower20Input()
{
	SpawnCoinTower(20);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower25Input()
{
	SpawnCoinTower(25);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower30Input()
{
	SpawnCoinTower(30);
}

void ACPCoinPusherItemSpawnTestPawn::SpawnCoinTower(int32 FloorCount)
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogCoinPusher, Warning, TEXT("Spawn Coin Tower (%d floors)"), FloorCount);
	TargetCoinPusher->SpawnTower(CoinTowerItemID, FloorCount);
}

void ACPCoinPusherItemSpawnTestPawn::HandleShowClearEndingInput()
{
	if (ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Show Ending - Clear"));
		PC->ShowEndingResult(true);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleShowLoseEndingInput()
{
	if (ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Show Ending - Lose"));
		PC->ShowEndingResult(false);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleDamagePlayerInput()
{
	PlayerHealth = FMath::Clamp(PlayerHealth - StatChangeAmount, 0.0f, PlayerMaxHealth);
	if (PlayerHealth <= 0.0f)
	{
		PlayerHealth = PlayerMaxHealth;
	}

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Player Health: %.0f / %.0f"), PlayerHealth, PlayerMaxHealth);

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->UpdatePlayerHealth(PlayerHealth, PlayerMaxHealth);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleGainPlayerExpInput()
{
	PlayerExp += StatChangeAmount;
	if (PlayerExp >= PlayerMaxExp)
	{
		PlayerExp = 0.0f;
	}

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Player Exp: %.0f / %.0f"), PlayerExp, PlayerMaxExp);

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->UpdatePlayerExp(PlayerExp, PlayerMaxExp);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleDamageBossInput()
{
	BossHealth = FMath::Clamp(BossHealth - StatChangeAmount, 0.0f, BossMaxHealth);
	if (BossHealth <= 0.0f)
	{
		BossHealth = BossMaxHealth;
	}

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Boss Health: %.0f / %.0f"), BossHealth, BossMaxHealth);

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->UpdateBossHealth(BossHealth, BossMaxHealth);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleGainBossExpInput()
{
	BossExp += StatChangeAmount;
	if (BossExp >= BossMaxExp)
	{
		BossExp = 0.0f;
	}

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Boss Exp: %.0f / %.0f"), BossExp, BossMaxExp);

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->UpdateBossExp(BossExp, BossMaxExp);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleGainTicketInput()
{
	++TicketCount;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Ticket Count: %d"), TicketCount);

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->UpdateTicketCount(TicketCount);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleGainComboInput()
{
	++ComboCount;
	ComboGaugeCurrent = ComboGaugeMax;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Combo: %d (gauge refilled)"), ComboCount);

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetComboCount(ComboCount);
		InGameWidget->UpdateComboGauge(ComboGaugeCurrent, ComboGaugeMax);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleResetComboInput()
{
	ComboCount = 0;
	ComboGaugeCurrent = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - Combo broken"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetComboCount(ComboCount);
		InGameWidget->UpdateComboGauge(ComboGaugeCurrent, ComboGaugeMax);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleTogglePauseInput()
{
	if (ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Toggle Pause Menu (legacy key, bypassing Enhanced Input)"));
		PC->TogglePauseMenu();
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleTogglePlayerInfoInput()
{
	bPlayerInfoVisible = !bPlayerInfoVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - PlayerInfoWidget visible: %s"), bPlayerInfoVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetPlayerInfoVisible(bPlayerInfoVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleBossInfoInput()
{
	bBossInfoVisible = !bBossInfoVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - BossInfoWidget visible: %s"), bBossInfoVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetBossInfoVisible(bBossInfoVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleBackgroundInput()
{
	bBackgroundVisible = !bBackgroundVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - BackgroundImage visible: %s"), bBackgroundVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetBackgroundVisible(bBackgroundVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleTicketCountInput()
{
	bTicketCountVisible = !bTicketCountVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - TicketCountWidget visible: %s"), bTicketCountVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetTicketCountVisible(bTicketCountVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleInventoryInput()
{
	bInventoryVisible = !bInventoryVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - InventoryWidget visible: %s"), bInventoryVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetInventoryVisible(bInventoryVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleCoinComboInput()
{
	bCoinComboVisible = !bCoinComboVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - CoinComboWidget visible: %s"), bCoinComboVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetCoinComboVisible(bCoinComboVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleRouletteInput()
{
	bRouletteVisible = !bRouletteVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - RouletteWidget visible: %s"), bRouletteVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetRouletteVisible(bRouletteVisible);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleToggleCoinPointUIInput()
{
	bCoinPointUIVisible = !bCoinPointUIVisible;

	UE_LOG(LogTemp, Warning, TEXT("InGameUI Test - CoinPointUI visible: %s"), bCoinPointUIVisible ? TEXT("true") : TEXT("false"));

	if (UCPInGameWidget* InGameWidget = GetInGameWidget())
	{
		InGameWidget->SetCoinPointUIVisible(bCoinPointUIVisible);
	}
}

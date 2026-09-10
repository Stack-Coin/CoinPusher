// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestPawn.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPPassiveCoinConvertArea.h"
#include "CoinPusher/CPCoinTowerSpawner.h"
#include "Roulette/CPRoulette.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"

void ACPCoinPusherItemSpawnTestPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetCoinPusher)
	{
		TargetCoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass()));
	}

	if (!TargetCoinPusher)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No ACPCoinPusher assigned or found in the level - Space/P/O/M/I/1-6 will do nothing."));
	}

	if (!TargetRoulette)
	{
		TargetRoulette = Cast<ACPRoulette>(UGameplayStatics::GetActorOfClass(this, ACPRoulette::StaticClass()));
	}

	if (!TargetRoulette)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No ACPRoulette assigned or found in the level - R will do nothing."));
	}
}

void ACPCoinPusherItemSpawnTestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleRollRouletteInput);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::O, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleHPConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleActiveWaveThrowInput);
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnBigCoinInput);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower5Input);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower10Input);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower15Input);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower20Input);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower25Input);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinTower30Input);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogTemp, Warning, TEXT("Coin Spawn"));
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
	UE_LOG(LogTemp, Warning, TEXT("Roll Roulette (started: %s)"), bStarted ? TEXT("true") : TEXT("false"));
}

void ACPCoinPusherItemSpawnTestPawn::HandleConvertActiveInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	if (ACPPassiveCoinConvertArea* ConvertArea = TargetCoinPusher->GetPassiveCoinConvertArea())
	{
		UE_LOG(LogTemp, Warning, TEXT("Convert Active"));
		ConvertArea->ConvertActive(PassiveConvertCount);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleHPConvertActiveInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	if (ACPPassiveCoinConvertArea* ConvertArea = TargetCoinPusher->GetPassiveCoinConvertArea())
	{
		UE_LOG(LogTemp, Warning, TEXT("HP Convert Active"));
		ConvertArea->HPConvertActive(HPConvertCount);
	}
}

void ACPCoinPusherItemSpawnTestPawn::HandleActiveWaveThrowInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Active Wave Throw"));
	TargetCoinPusher->ActiveWaveThrow();
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnBigCoinInput()
{
	if (!TargetCoinPusher)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Spawn Big Coin"));
	TargetCoinPusher->SpawnBigCoin();
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

	if (ACPCoinTowerSpawner* CoinTowerSpawner = TargetCoinPusher->GetCoinTowerSpawner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn Coin Tower (%d floors)"), FloorCount);
		CoinTowerSpawner->SpawnTower(FloorCount);
	}
}

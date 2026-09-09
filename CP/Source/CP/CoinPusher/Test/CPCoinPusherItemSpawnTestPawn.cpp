// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestPawn.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPPassiveCoinConvertArea.h"
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
		UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No ACPCoinPusher assigned or found in the level - Space/P/O/M/I will do nothing."));
	}
}

void ACPCoinPusherItemSpawnTestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::O, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleHPConvertActiveInput);
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleActiveWaveThrowInput);
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnBigCoinInput);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogTemp, Warning, TEXT("Coin Spawn"));
		TargetCoinPusher->ItemSpawn(CoinItemID, 1);
	}
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

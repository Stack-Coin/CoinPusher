// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherItemSpawnTestPawn.h"
#include "CoinPusher/CPCoinPusher.h"
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
		UE_LOG(LogTemp, Warning, TEXT("[ACPCoinPusherItemSpawnTestPawn] No ACPCoinPusher assigned or found in the level - Space will do nothing."));
	}
}

void ACPCoinPusherItemSpawnTestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput);
}

void ACPCoinPusherItemSpawnTestPawn::HandleSpawnCoinInput()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogTemp, Warning, TEXT("Coin Spawn"));
		TargetCoinPusher->ItemSpawn(CoinItemID, 1);
	}
}

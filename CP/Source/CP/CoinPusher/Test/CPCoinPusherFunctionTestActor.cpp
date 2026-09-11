// Fill out your copyright notice in the Description page of Project Settings.

#include "CPCoinPusherFunctionTestActor.h"
#include "CoinPusher/CPCoinPusher.h"
#include "Roulette/CPRoulette.h"
#include "Log/CPLogCategories.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ACPCoinPusherFunctionTestActor::ACPCoinPusherFunctionTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACPCoinPusherFunctionTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetCoinPusher)
	{
		TargetCoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass()));
	}

	if (!TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPCoinPusherFunctionTestActor] No ACPCoinPusher assigned or found in the level - Numpad 1-6/8 will do nothing."));
	}

	if (!TargetRoulette)
	{
		TargetRoulette = Cast<ACPRoulette>(UGameplayStatics::GetActorOfClass(this, ACPRoulette::StaticClass()));
	}

	if (!TargetRoulette)
	{
		UE_LOG(LogRoulette, Warning, TEXT("[ACPCoinPusherFunctionTestActor] No ACPRoulette assigned or found in the level - Numpad 7 will do nothing."));
	}

	// 다른 Pawn이 possess되어 있어도 상관없이 항상 넘버패드 입력을 받도록, 이 Actor 자신의
	// InputComponent를 PlayerController의 입력 스택에 직접 얹는다 (Pawn의 SetupPlayerInputComponent와
	// 달리, 이 방식은 possess 여부와 완전히 무관하게 동작한다)
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[ACPCoinPusherFunctionTestActor] No local PlayerController found - numpad input will not work."));
		return;
	}

	EnableInput(PlayerController);

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleSpawnNormalCoin);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleSpawnBigCoin);
	InputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleConvertPassive);
	InputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleConvertHP);
	InputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleConvertMonster);
	InputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleSpawnMonsterCoin);
	InputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleRollRoulette);
	InputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &ACPCoinPusherFunctionTestActor::HandleSpawnCoinTower);
}

void ACPCoinPusherFunctionTestActor::HandleSpawnNormalCoin()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad1] Spawn Normal Coin"));
		TargetCoinPusher->ItemSpawn(NormalCoinItemID, 1);
	}
}

void ACPCoinPusherFunctionTestActor::HandleSpawnBigCoin()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad2] Spawn Big Coin"));
		TargetCoinPusher->SpawnBigCoin(BigCoinItemID, 1);
	}
}

void ACPCoinPusherFunctionTestActor::HandleConvertPassive()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad3] Convert Active (Passive x%d)"), PassiveConvertCount);
		TargetCoinPusher->ConvertActive(PassiveConvertItemID, PassiveConvertCount);
	}
}

void ACPCoinPusherFunctionTestActor::HandleConvertHP()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad4] HP Convert Active (x%d)"), HPConvertCount);
		TargetCoinPusher->HPConvertActive(HPConvertItemID, HPConvertCount);
	}
}

void ACPCoinPusherFunctionTestActor::HandleConvertMonster()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad5] Monster Convert Active (x%d)"), MonsterConvertCount);
		TargetCoinPusher->MonsterConvertActive(MonsterConvertItemID, MonsterConvertCount);
	}
}

void ACPCoinPusherFunctionTestActor::HandleSpawnMonsterCoin()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad6] Spawn Monster Coin (x%d)"), MonsterCoinSpawnCount);
		TargetCoinPusher->SpawnMonsterCoin(MonsterCoinItemID, MonsterCoinSpawnCount);
	}
}

void ACPCoinPusherFunctionTestActor::HandleRollRoulette()
{
	if (!TargetRoulette)
	{
		return;
	}

	const bool bStarted = TargetRoulette->Roll();
	UE_LOG(LogRoulette, Warning, TEXT("[Numpad7] Roll Roulette (started: %s)"), bStarted ? TEXT("true") : TEXT("false"));
}

void ACPCoinPusherFunctionTestActor::HandleSpawnCoinTower()
{
	if (TargetCoinPusher)
	{
		UE_LOG(LogCoinPusher, Warning, TEXT("[Numpad8] Spawn Coin Tower (%d floors)"), CoinTowerFloorCount);
		TargetCoinPusher->SpawnTower(CoinTowerItemID, CoinTowerFloorCount);
	}
}

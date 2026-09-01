// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDispenser.h"
#include "CPCoin.h"
#include "CPInput.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ACPDispenser::ACPDispenser()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetCollisionProfileName(FName("BlockAllDynamic"));
	Body->bNavigationRelevant = false;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(RootComponent);
}

void ACPDispenser::BeginPlay()
{
	Super::BeginPlay();

	//Input에 Delegate에 등록 (ACPCoinPusher가 미리 SetLinkedInput()으로 연결해 둔 경우 여기서는 이미 바인딩되어 있음)
	BindToLinkedInput();
}

void ACPDispenser::DispenseCoin()
{
	if (!CoinClass || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (ACPCoin* Coin = GetWorld()->SpawnActor<ACPCoin>(CoinClass, SpawnPoint->GetComponentLocation(), SpawnPoint->GetComponentRotation(), SpawnParams))
	{
		const FVector LaunchVelocity = SpawnPoint->GetForwardVector() * LaunchForwardSpeed + FVector::UpVector * LaunchUpwardSpeed;
		Coin->Launch(LaunchVelocity);
	}
}

void ACPDispenser::SetLinkedInput(ACPInput* NewLinkedInput)
{
	if (LinkedInput == NewLinkedInput)
	{
		return;
	}

	//기존에 연결되어 있던 Input의 델리게이트는 해제
	if (LinkedInput)
	{
		LinkedInput->OnInteracted.RemoveDynamic(this, &ACPDispenser::HandleInputInteracted);
	}

	LinkedInput = NewLinkedInput;

	BindToLinkedInput();
}

void ACPDispenser::HandleInputInteracted(AActor* Interactor)
{
	DispenseCoin();
}

void ACPDispenser::BindToLinkedInput()
{
	if (LinkedInput)
	{
		LinkedInput->OnInteracted.AddUniqueDynamic(this, &ACPDispenser::HandleInputInteracted);
	}
}

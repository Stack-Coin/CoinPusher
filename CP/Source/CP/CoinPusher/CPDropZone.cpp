// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDropZone.h"
#include "CPCoin.h"
#include "Components/BoxComponent.h"

ACPDropZone::ACPDropZone()
{
	PrimaryActorTick.bCanEverTick = false;


	RootComponent = CollectionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollectionVolume"));

	// Box크기
	CollectionVolume->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));


	CollectionVolume->SetCollisionProfileName(FName("OverlapAllDynamic"));

	//overlap함수 등록
	CollectionVolume->OnComponentBeginOverlap.AddDynamic(this, &ACPDropZone::OnVolumeBeginOverlap);
}

void ACPDropZone::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Coin이 떨어졌는가?
	if (ACPCoin* Coin = Cast<ACPCoin>(OtherActor))
	{
		CollectedCoinCount++;

		OnCoinCollected.Broadcast(Coin, CollectedCoinCount);

		Coin->Collect();
	}
}

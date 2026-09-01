// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDropZone.h"
#include "CPCoinPusherItem.h"
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

void ACPDropZone::AddCollectedCoins(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CollectedCoinCount += Amount;

	OnCoinCollected.Broadcast(CollectedCoinCount);
}

void ACPDropZone::RecordCollectedItem(FName ItemCode)
{
	if (ItemCode.IsNone())
	{
		return;
	}

	CollectedItemCodes.Add(ItemCode);

	OnItemCollected.Broadcast(ItemCode);
}

void ACPDropZone::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//ICPCoinPusherItem을 구현하는 오브젝트(Coin, Item ...)가 떨어졌는가?
	if (ICPCoinPusherItem* Item = Cast<ICPCoinPusherItem>(OtherActor))
	{
		Item->OnDroppedInZone(this);
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDropZone.h"
#include "CPCoinPusherItem.h"
#include "CPDispenser.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Player/CPGameMode.h"

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

	//코인이 10개 모일 때마다(예: 10, 20, 30...) GameMode를 찾아 떨어진 코인 수량을 넘겨줌
	if (CollectedCoinCount % 10 == 0)
	{
		/*
		if (ACPGameMode* GameMode = GetWorld() ? Cast<ACPGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			//TODO: ACPGameMode에 UpdateTicketCount가 추가되면 연결
			//GameMode->UpdateTicketCount(CollectedCoinCount);
		}
		*/
	}
}

void ACPDropZone::RecordCollectedItem(FName ItemCode)
{
	if (ItemCode.IsNone())
	{
		return;
	}

	CollectedItemCodes.Add(ItemCode);

	OnItemCollected.Broadcast(ItemCode);

	//레벨(CoinPusher)에서 지정해 둔 Dispenser가 있으면 그쪽에 같은 ItemID의 재생성을 요청
	if (ItemRespawnDispenser)
	{
		ItemRespawnDispenser->DispenseItemByID(ItemCode, 1, ECPDispenserSpawnType::WorldItem);
	}
}

void ACPDropZone::SetItemRespawnDispenser(ACPDispenser* NewItemRespawnDispenser)
{
	ItemRespawnDispenser = NewItemRespawnDispenser;
}

void ACPDropZone::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//ICPCoinPusherItem을 구현하는 오브젝트(Coin, Item ...)가 떨어졌는가?
	if (ICPCoinPusherItem* Item = Cast<ICPCoinPusherItem>(OtherActor))
	{
		Item->OnDroppedInZone(this);
	}
}

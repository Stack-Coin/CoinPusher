// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDropZone.h"
#include "CPCoinPusherItem.h"
#include "CPDispenser.h"
#include "CPDroppedItemReceiver.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CPGameMode.h"
#include "Player/CPPlayerCharacter.h"

ACPDropZone::ACPDropZone()
{
	PrimaryActorTick.bCanEverTick = false;


	RootComponent = CollectionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollectionVolume"));

	// Boxũ��
	CollectionVolume->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));


	CollectionVolume->SetCollisionProfileName(FName("OverlapAllDynamic"));

	//overlap�Լ� ���
	CollectionVolume->OnComponentBeginOverlap.AddDynamic(this, &ACPDropZone::OnVolumeBeginOverlap);
}

void ACPDropZone::AddCollectedCoins(int32 Amount, FName ItemID, ECPCoinType CoinType)
{
	if (Amount <= 0)
	{
		return;
	}

	CollectedCoinCount += Amount;

	OnCoinCollected.Broadcast(CollectedCoinCount);

	//떨어진 아이템의 정보(ItemID/개수, 코인이면 CoinType까지)를 GameMode로 전달.
	//GetAuthGameMode()가 ICPDroppedItemReceiver를 구현하는 경우에만 전달되므로, 실제 게임의 GameMode든
	//테스트용 GameMode든 이 인터페이스만 구현하면 받을 수 있다
	if (!ItemID.IsNone())
	{
		if (ICPDroppedItemReceiver* Receiver = GetWorld() ? Cast<ICPDroppedItemReceiver>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			Receiver->ReceiveDroppedItem(ItemID, Amount, CoinType);
		}
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

	//CoinPusher가 지정해둔 재생성 담당 Dispenser에게 같은 ItemID로 재생성 요청
	if (ItemRespawnDispenser)
	{
		ItemRespawnDispenser->DispenseItemByID(ItemCode, 1, ECPDispenserSpawnType::WorldItem);
	}

	//떨어진 아이템의 정보를 GameMode로 전달 (Item은 코인이 아니므로 CoinType은 기본값 Normal)
	if (ICPDroppedItemReceiver* Receiver = GetWorld() ? Cast<ICPDroppedItemReceiver>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		Receiver->ReceiveDroppedItem(ItemCode, 1);
	}
}

void ACPDropZone::SetItemRespawnDispenser(ACPDispenser* NewItemRespawnDispenser)
{
	ItemRespawnDispenser = NewItemRespawnDispenser;
}

void ACPDropZone::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//ICPCoinPusherItem�� �����ϴ� ������Ʈ(Coin, Item ...)�� �������°�?
	if (ICPCoinPusherItem* Item = Cast<ICPCoinPusherItem>(OtherActor))
	{
		Item->OnDroppedInZone(this);
	}
}

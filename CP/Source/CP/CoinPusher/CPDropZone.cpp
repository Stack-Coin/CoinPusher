// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPDropZone.h"
#include "CPCoin.h"
#include "CPItem.h"
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
		OnDropped.Broadcast(ItemID);

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
	OnDropped.Broadcast(ItemCode);

	//CoinPusher가 지정해둔 재생성 담당 Dispenser에게 같은 ItemID로 재생성 요청
	if (ItemRespawnDispenser)
	{
		ItemRespawnDispenser->DispenseItemByID(ItemCode, 1);
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
	// 코인/아이템에게 자기 데이터를 보고하게 하지 않고, DropZone이 직접 구체 타입으로 캐스팅해서
	// ItemID(코인은 CoinType까지)를 조회한 뒤 기록한다. Collect()가 false를 반환하면(이미 수집된
	// 상태) 기록하지 않음 - 오버랩이 중복으로 들어와도 같은 코인/아이템이 두 번 집계되지 않게 함
	if (ACPCoin* Coin = Cast<ACPCoin>(OtherActor))
	{
		const FName ItemID = Coin->GetItemID();
		const ECPCoinType CoinType = Coin->GetCoinType();

		if (Coin->Collect())
		{
			AddCollectedCoins(1, ItemID, CoinType);
		}
	}
	else if (ACPItem* Item = Cast<ACPItem>(OtherActor))
	{
		const FName ItemCode = Item->GetItemId();

		if (Item->Collect())
		{
			RecordCollectedItem(ItemCode);
		}
	}
}

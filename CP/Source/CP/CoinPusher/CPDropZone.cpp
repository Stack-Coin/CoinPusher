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

	// Grants ExperiencePerCoin * Amount experience to the team - experience is shared/team-owned (ACPGameMode), not per player
	if (ExperiencePerCoin != 0.0f)
	{
		if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
		{
			GameMode->AddTeamExperience(ExperiencePerCoin * static_cast<float>(Amount));
		}
	}

	//코인이 10개 모일 때마다(예: 10, 20, 30...) GameMode를 찾아 팀에게 티켓 1개를 지급
	if (CollectedCoinCount % CoinsPerTicket == 0)
	{
		if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
		{
			GameMode->AddTeamTickets(1);
		}
	}

	//떨어진 아이템의 정보(ItemID/개수, 코인이면 CoinType까지)를 GameMode로 전달
	if (!ItemID.IsNone())
	{
		if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
		{
			GameMode->ReceiveDroppedItem(ItemID, Amount, CoinType);
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
	if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
	{
		GameMode->ReceiveDroppedItem(ItemCode, 1);
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

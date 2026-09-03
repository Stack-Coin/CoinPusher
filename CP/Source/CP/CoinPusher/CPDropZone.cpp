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

void ACPDropZone::AddCollectedCoins(int32 Amount)
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

	//������ 10�� ���� ������(��: 10, 20, 30...) GameMode�� ã�� ������ ���� ������ �Ѱ���
	if (CollectedCoinCount % CoinsPerTicket == 0)
	{
		if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
		{
			GameMode->AddTeamTickets(1);
		}

		/*
		if (ACPGameMode* GameMode = GetWorld() ? Cast<ACPGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			//TODO: ACPGameMode�� UpdateTicketCount�� �߰��Ǹ� ����
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

	//����(CoinPusher)���� ������ �� Dispenser�� ������ ���ʿ� ���� ItemID�� ������� ��û
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
	//ICPCoinPusherItem�� �����ϴ� ������Ʈ(Coin, Item ...)�� �������°�?
	if (ICPCoinPusherItem* Item = Cast<ICPCoinPusherItem>(OtherActor))
	{
		Item->OnDroppedInZone(this);
	}
}

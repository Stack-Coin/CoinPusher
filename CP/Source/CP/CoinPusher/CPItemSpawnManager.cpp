#include "CoinPusher/CPItemSpawnManager.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPItemSpawnZone.h"
#include "Datatables/CPItemData.h"
#include "Engine/DataTable.h"

ACPItemSpawnManager::ACPItemSpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACPItemSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (FOnCPDropZoneDropped* DropZoneDropped = CoinPusher ? CoinPusher->GetDropZoneDroppedDelegate() : nullptr)
	{
		DropZoneDropped->AddDynamic(this, &ACPItemSpawnManager::HandleDropZoneItemDropped);

	}
}

void ACPItemSpawnManager::HandleDropZoneItemDropped(FName ItemID)
{
	UE_LOG(LogTemp, Warning, TEXT("Spawnzone"));
	UDataTable* ItemDataTable = CoinPusher ? CoinPusher->GetItemDataTable() : nullptr;
	const FItemData* Row = ItemDataTable
		? ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ACPItemSpawnManager::HandleDropZoneItemDropped"))
		: nullptr;

	if (!Row || !Row->WorldSpawnBPClass || !SpawnableCategories.Contains(Row->Category))
	{
	
		return;
	}

	TArray<ACPItemSpawnZone*> ValidZones;
	ValidZones.Reserve(SpawnZones.Num());
	for (ACPItemSpawnZone* Zone : SpawnZones)
	{
		if (Zone)
		{
		
			ValidZones.Add(Zone);
		}
	}

	if (ValidZones.Num() == 0)
	{

		return;
	}

	ACPItemSpawnZone* SpawnZone = ValidZones[FMath::RandRange(0, ValidZones.Num() - 1)];

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UE_LOG(LogTemp, Warning, TEXT("SpawnZone=%s Class=%s Loc=%s"),
		*SpawnZone->GetName(), *Row->WorldSpawnBPClass->GetName(), *SpawnZone->GetActorLocation().ToString());

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(Row->WorldSpawnBPClass, SpawnZone->GetActorLocation(), SpawnZone->GetActorRotation(), SpawnParams);

	UE_LOG(LogTemp, Warning, TEXT("SpawnedActor=%s"), SpawnedActor ? *SpawnedActor->GetName() : TEXT("NULL"));
}

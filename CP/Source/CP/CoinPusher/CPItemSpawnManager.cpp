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
	UDataTable* ItemDataTable = CoinPusher ? CoinPusher->GetItemDataTable() : nullptr;
	const FItemData* Row = ItemDataTable
		? ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ACPItemSpawnManager::HandleDropZoneItemDropped"))
		: nullptr;

	if (!Row || !Row->WorldSpawnBPClass || !SpawnableCategories.Contains(Row->Category))
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawnzone"));
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

	FVector SpawnLocation = SpawnZone->GetActorLocation();
	if (SpawnPositionJitterRadius > 0.0f)
	{
		// 같은 SpawnZone에 연달아 스폰돼도 정확히 같은 좌표에 겹치지 않도록 원형 범위 내에서 무작위로 흔든다
		const float JitterAngle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float JitterDistance = FMath::FRandRange(0.0f, SpawnPositionJitterRadius);
		SpawnLocation += FVector(FMath::Cos(JitterAngle) * JitterDistance, FMath::Sin(JitterAngle) * JitterDistance, 0.0f);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UE_LOG(LogTemp, Warning, TEXT("SpawnZone=%s Class=%s Loc=%s"),
		*SpawnZone->GetName(), *Row->WorldSpawnBPClass->GetName(), *SpawnLocation.ToString());

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(Row->WorldSpawnBPClass, SpawnLocation, SpawnZone->GetActorRotation(), SpawnParams);

	UE_LOG(LogTemp, Warning, TEXT("SpawnedActor=%s"), SpawnedActor ? *SpawnedActor->GetName() : TEXT("NULL"));
}

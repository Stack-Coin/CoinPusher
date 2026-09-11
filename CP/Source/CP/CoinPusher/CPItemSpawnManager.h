// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPItemSpawnManager.generated.h"

class ACPCoinPusher;
class ACPItemSpawnZone;

/** ACPCoinPusher의 DropZone(OnDropped)을 구독해, 드랍된 아이템이 ItemDataTable(FItemData) 기준으로
 *  SpawnableCategories에 속하면 SpawnZones 중 하나를 랜덤으로 골라 그 아이템의 WorldSpawnBPClass를
 *  월드에 스폰한다. CoinPusher/SpawnZones 모두 레벨에서 직접 배치 후 Details에서 수동으로 할당한다 */
UCLASS(abstract)
class CP_API ACPItemSpawnManager : public AActor
{
	GENERATED_BODY()

public:

	ACPItemSpawnManager();

protected:

	/** OnDropped를 구독할 CoinPusher. 레벨에서 직접 할당 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Item Spawn")
	TObjectPtr<ACPCoinPusher> CoinPusher;

	/** 월드 스폰 후보 지점들 - 수동 등록. 레벨에 ItemSpawnZone들을 배치한 뒤 이 배열에 직접 추가 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Item Spawn")
	TArray<TObjectPtr<ACPItemSpawnZone>> SpawnZones;

	/** ItemDataTable에서 조회한 FItemData::Category가 이 목록에 있으면 월드 스폰 대상으로 취급 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Spawn")
	TArray<FName> SpawnableCategories = { FName("Sword"), FName("StatUp") };

	virtual void BeginPlay() override;

	/** CoinPusher->GetDropZoneDroppedDelegate()에 바인딩 - ItemDataTable에서 ItemID를 조회해 Category가
	 *  SpawnableCategories에 있으면 SpawnZones 중 하나를 랜덤으로 골라 Row.WorldSpawnBPClass를 스폰 */
	UFUNCTION()
	void HandleDropZoneItemDropped(FName ItemID);
};

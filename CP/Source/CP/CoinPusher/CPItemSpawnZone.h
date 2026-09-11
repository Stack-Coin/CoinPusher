// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPItemSpawnZone.generated.h"

class USceneComponent;

/** ACPItemSpawnManager가 월드 아이템을 스폰할 위치/방향을 표시하는 마커 액터.
 *  레벨에 원하는 위치마다 배치해두고, ACPItemSpawnManager::SpawnZones 배열에 수동으로 등록한다 */
UCLASS(abstract)
class CP_API ACPItemSpawnZone : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SpawnPoint;

public:

	ACPItemSpawnZone();

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }
};

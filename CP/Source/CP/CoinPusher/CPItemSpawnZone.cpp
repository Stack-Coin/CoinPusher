#include "CoinPusher/CPItemSpawnZone.h"
#include "Components/SceneComponent.h"

ACPItemSpawnZone::ACPItemSpawnZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	RootComponent = SpawnPoint;
}

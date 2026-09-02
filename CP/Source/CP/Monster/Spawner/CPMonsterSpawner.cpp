// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "../CPMonsterBase.h"

// Sets default values
ACPMonsterSpawner::ACPMonsterSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ACPMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (bShouldSpawnEnemiesImmediately)
	{
		// schedule the first enemy spawn
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ACPMonsterSpawner::SpawnEnemy, InitialSpawnDelay);
	}
}

void ACPMonsterSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}

void ACPMonsterSpawner::ActivateInteraction(AActor* ActivationInstigator)
{
	// ensure we're only activated once, and only if we've deferred enemy spawning
	if (bHasBeenActivated || bShouldSpawnEnemiesImmediately)
	{
		return;
	}

	// raise the activation flag
	bHasBeenActivated = true;

	// spawn the first enemy
	SpawnEnemy();
}

void ACPMonsterSpawner::SpawnEnemy()
{
	if (IsValid(EnemyClass))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ACPMonsterBase* SpawnedEnemy = GetWorld()->SpawnActor<ACPMonsterBase>(EnemyClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

		// was the enemy successfully created?
		if (SpawnedEnemy)
		{
			// subscribe to the death delegate
			SpawnedEnemy->OnMonsterDied.AddDynamic(this, &ACPMonsterSpawner::OnEnemyDied);
		}
	}
}

void ACPMonsterSpawner::OnEnemyDied()
{
	for (AActor* CurrentActor : ActorsToActivateWhenDepleted)
	{
		if (ICPMonsterSpawnInterface* MonsterSpawn = Cast<ICPMonsterSpawnInterface>(CurrentActor))
		{
			// activate the actor
			MonsterSpawn->ActivateInteraction(this);
		}
	}
}

void ACPMonsterSpawner::SpawnerDepleted()
{
	// process the actors to activate list
	for (AActor* CurrentActor : ActorsToActivateWhenDepleted)
	{
		// check if the actor is activatable
		if (ICPMonsterSpawnInterface* MonsterSpawn = Cast<ICPMonsterSpawnInterface>(CurrentActor))
		{
			// activate the actor
			MonsterSpawn->ActivateInteraction(this);
		}
	}
}

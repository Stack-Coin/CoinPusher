// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPMonsterSpawnInterface.h"
#include "CPMonsterSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class ACPMonsterBase;

UCLASS()
class CP_API ACPMonsterSpawner : public AActor, public ICPMonsterSpawnInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACPMonsterSpawner();

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:
	//UFUNCTION(BlueprintCallable, Category = "Activatable")
	//virtual void ToggleInteraction(AActor* ActivationInstigator) override;

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	virtual void ActivateInteraction(AActor* ActivationInstigator) override;

	//UFUNCTION(BlueprintCallable, Category = "Activatable")
	//virtual void DeactivateInteraction(AActor* ActivationInstigator) override;

protected:	
	void SpawnEnemy();

	UFUNCTION()
	void OnEnemyDied();

	void SpawnerDepleted();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	TSubclassOf<ACPMonsterBase> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	bool bShouldSpawnEnemiesImmediately = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float InitialSpawnDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = 0, ClampMax = 100))
	int32 SpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float RespawnDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation", meta = (ClampMin = 0, ClampMax = 10))
	float ActivationDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation")
	TArray<AActor*> ActorsToActivateWhenDepleted;

	bool bHasBeenActivated = false;

	FTimerHandle SpawnTimer;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPMonsterSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class ACPMonsterBase;

UCLASS()
class CP_API ACPMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACPMonsterSpawner();

public:
	//이 스포너 위치를 중심으로 InCount마리를 Y축(스포너가 바라보는 방향의 좌우)으로 나란히 스폰합니다.
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	ACPMonsterBase* SpawnMonsterRow(TSubclassOf<ACPMonsterBase> MonsterClass, int32 InCount, float InRowSpacingY, int32 InWave);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;
};

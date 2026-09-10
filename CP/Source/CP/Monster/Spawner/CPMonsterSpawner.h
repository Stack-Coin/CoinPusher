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
	//InCount마리 전부(1마리 이상이어도 전부)를 반환합니다 - 호출부가 스폰된 각 몬스터의 죽음 델리게이트를
	//개별 구독해야 하는 경우(전멸 감지 등)를 위해 마지막 1마리만 반환하던 이전 방식에서 바꿈.
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	TArray<ACPMonsterBase*> SpawnMonsterRow(TSubclassOf<ACPMonsterBase> MonsterClass, int32 InCount, float InRowSpacingY, int32 InRound, int32 InWave);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;
};

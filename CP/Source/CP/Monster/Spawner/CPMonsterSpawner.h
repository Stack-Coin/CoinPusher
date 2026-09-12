// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPMonsterSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class ACPMonsterBase;
struct FNavAgentProperties;

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

protected:
	/** InDesiredLocation이 다른 몬스터/장애물과 겹치면, 그 주변을 원형으로 훑어서 비어있는 자리를 찾아 반환함.
	 *  전부 막혀있으면 원래 위치를 그대로 반환함 - 이 경우 실제 스폰은 SpawnActor의
	 *  AdjustIfPossibleButAlwaysSpawn 옵션이 최후 보정을 시도함(스폰 자체가 실패하는 일은 없음) */
	FVector ResolveFreeSpawnLocation(const FVector& InDesiredLocation, const FNavAgentProperties& InNavAgentProps) const;

	/** InLocation을 NavMesh 위의 가장 가까운 유효 위치로 투영함. 투영 범위 밖(NavMesh 자체가 없음)이면
	 *  원래 위치를 그대로 반환함. InNavAgentProps로 스폰될 몬스터 크기에 맞는 NavMesh(Supported Agent)를
	 *  골라서 투영함 - Boss처럼 큰 몬스터가 일반 몬스터용 좁은 NavMesh에 투영되는 걸 방지 */
	FVector ProjectToNavMesh(const FVector& InLocation, const FNavAgentProperties& InNavAgentProps) const;

public:
	/** ResolveFreeSpawnLocation에서 겹침 검사에 쓰는 구체 반경(cm) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	float OverlapCheckRadius = 50.f;

	/** 원래 위치가 막혀있을 때 대신 시도해볼 후보 위치 개수 (원래 위치 주변을 이 개수만큼 등분해서 훑음) */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = 1))
	int32 MaxRelocationAttempts = 4;

	/** 후보 위치를 원래 위치로부터 얼마나 떨어뜨려서 시도할지(cm) */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = 0))
	float RelocationStepDistance = 60.f;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;
};

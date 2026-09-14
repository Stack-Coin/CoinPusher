// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Monster/Stat/CPMonsterStatTypes.h"
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
	/** InMonsterType은 몬스터 풀(UCPMonsterPoolSubsystem) 조회 키로만 씀 - MonsterClass의 CDO에서
	 *  재추론하지 않고, 호출부(RoundInfo/WaveInfo)가 이미 알고 있는 권위있는 값을 그대로 받음.
	 *  CDO의 MonsterType 필드가 실제 값과 다르게 세팅돼있어도(BP 설정 누락 등) 풀이 엉뚱한 타입으로
	 *  섞이지 않도록 하기 위함.
	 *  bAllowFallbackOutsideNavMesh가 false(기본)면 이 행의 몬스터는 NavMesh 투영에 실패한 자리에
	 *  스폰을 시도하지 않고 그 마리만 건너뜀(검증 안 된 위치에 몬스터를 두지 않기 위함 - AI가 NavMesh
	 *  밖에서 먹통이 되는 문제 방지). true면(보스 전용) 실패해도 원래 위치에 강제로라도 스폰함 - 보스는
	 *  절대 스폰 자체가 스킵되면 안 되므로 */
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	TArray<ACPMonsterBase*> SpawnMonsterRow(TSubclassOf<ACPMonsterBase> MonsterClass, ECPMonsterType InMonsterType, int32 InCount, float InRowSpacingY, int32 InRound, int32 InWave, const FVector& InPlayerLocation, bool bAllowFallbackOutsideNavMesh = false);

protected:
	/** InDesiredLocation이 다른 몬스터/장애물과 겹치거나 플레이어(InPlayerLocation)와 MinPlayerSpawnDistance보다
	 *  가까우면, 그 주변을 원형으로 훑어서 비어있는 자리를 찾음. 찾은 자리(또는 전부 막혀서 원래 위치)가
	 *  NavMesh에 실제로 투영되고 아레나 중심까지 실제로 길이 이어지면(IsLocationReachableFromArenaCenter)
	 *  OutLocation에 채우고 true 반환. 그마저 다 실패하면(주변에 NavMesh가 전혀 없음) false를 반환 - 호출부가
	 *  이 자리를 스폰 후보에서 제외할지 판단하는 근거로 씀(검증 안 된 위치를 검증된 것처럼 속이지 않기 위함) */
	bool ResolveFreeSpawnLocation(const FVector& InDesiredLocation, const FNavAgentProperties& InNavAgentProps, const FVector& InPlayerLocation, FVector& OutLocation) const;

	/** InLocation을 NavMesh 위의 가장 가까운 유효 위치로 투영해 OutLocation에 채우고 true 반환.
	 *  투영 범위 밖(NavMesh 자체가 없음)이면 OutLocation을 건드리지 않고 false 반환 - 호출부가 이
	 *  실패를 "검증 안 됨"으로 취급하도록 함(예전처럼 원래 위치를 검증된 것처럼 돌려주지 않음).
	 *  InNavAgentProps로 스폰될 몬스터 크기에 맞는 NavMesh(Supported Agent)를 골라서 투영함 -
	 *  Boss처럼 큰 몬스터가 일반 몬스터용 좁은 NavMesh에 투영되는 걸 방지.
	 *  InExtentXY/InExtentZ로 탐색 범위를 조절함 - 보스 전용 2단계(넓은 범위 재탐색)에서 기본값보다
	 *  훨씬 넓게 줘서 씀(SpawnMonsterRow 참고) */
	bool ProjectToNavMesh(const FVector& InLocation, const FNavAgentProperties& InNavAgentProps, FVector& OutLocation, float InExtentXY = 200.f, float InExtentZ = 200.f) const;

	/** InLocation에서 ArenaCenterLocation까지 NavMesh 경로가 끊기지 않고 이어지는지 확인.
	 *  NavMesh 투영은 "가장 가까운 점"만 찾을 뿐 그 점이 본섬인지 고립된 조각(island)인지는 구분 못 하므로,
	 *  투영에 성공한 위치라도 실제로 갈 수 있는 곳인지 이 함수로 한 번 더 검증함 - 보스가 스폰은 됐는데
	 *  그 자리에 갇혀 못 움직이는 문제(끼임) 방지용. NavSys가 없으면(월드에 NavMesh 자체가 없음) 판단을
	 *  포기하고 true를 돌려줌 - 이 경우는 ProjectToNavMesh가 이미 앞단에서 걸러줌 */
	bool IsLocationReachableFromArenaCenter(const FVector& InLocation, const FNavAgentProperties& InNavAgentProps) const;

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

	/** 스폰 위치가 플레이어와 이 거리(cm)보다 가까우면 재탐색함. 0이면 플레이어 근접 여부를 검사하지 않음 */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = 0))
	float MinPlayerSpawnDistance = 300.f;

	/** 이 스포너가 속한 아레나의 중심 위치. UCPMonsterSpawnManagerComponent::CreateSpawnerRing이 스포너를
	 *  생성한 직후 채워줌 - IsLocationReachableFromArenaCenter가 "고립된 NavMesh 조각" 판정 기준으로 씀 */
	UPROPERTY(VisibleAnywhere, Category = "Spawn")
	FVector ArenaCenterLocation = FVector::ZeroVector;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;
};

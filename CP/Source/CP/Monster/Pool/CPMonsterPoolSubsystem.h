// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Monster/Stat/CPMonsterStatTypes.h"
#include "CPMonsterPoolSubsystem.generated.h"

class ACPMonsterBase;

USTRUCT()
struct FCPMonsterPool
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<ACPMonsterBase> MonsterClass;

	UPROPERTY()
	TArray<TObjectPtr<ACPMonsterBase>> InactiveActors;

	/** WarmUp()/Release()가 채워줌(그 라운드에서 기대되는 최대 동시 마릿수) - 이 이상은 Release() 시
	 *  풀에 쌓지 않고 Destroy() 함 */
	UPROPERTY()
	int32 MaxPoolSize = 0;
};

/**
 * 몬스터 타입(ECPMonsterType)별 오브젝트 풀. 웨이브마다 반복되는 SpawnActor/Destroy를 없애기 위해,
 * 죽은 몬스터를 파괴하는 대신 비활성화해서 재사용함(ACPMonsterBase::OnReturnedToPool/OnAcquiredFromPool 참고).
 * WorldSubsystem이라 레벨 시작/종료 시 자동으로 생성/정리됨 - 별도 초기화 필요 없음.
 */
UCLASS()
class CP_API UCPMonsterPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 라운드 시작 시 호출 - InCount만큼 미리 스폰해서 비활성 상태로 채워둠(이미 그만큼 있으면 아무것도
	 *  안 함, 모자라면 부족한 만큼만 추가). MaxPoolSize도 InCount 기준으로 갱신됨(늘어나기만 함).
	 *  InCount가 크면(수백 마리) 한 프레임에 몰아서 스폰하지 않고 WarmUpBatchSize만큼씩 나눠 프레임을
	 *  넘겨가며 채움(WarmUpBudgeted 참고) - 라운드 시작 시점의 히치를 막기 위함 */
	void WarmUp(ECPMonsterType Type, TSubclassOf<ACPMonsterBase> MonsterClass, int32 InCount);

	/** 풀에 재사용 가능한 액터가 있으면 꺼내서 Transform 위치로 재배치 후 반환, 없으면 새로 스폰함 */
	ACPMonsterBase* Acquire(ECPMonsterType Type, TSubclassOf<ACPMonsterBase> MonsterClass, const FTransform& Transform);

	/** Monster를 비활성화해서 풀에 반환함(상한을 넘으면 대신 Destroy) */
	void Release(ACPMonsterBase* Monster);

private:
	/** WarmUp()의 실제 작업 - 한 번 호출에 WarmUpBatchSize만큼만 스폰하고, 아직 TargetCount에 못
	 *  미치면 다음 프레임에 스스로를 다시 예약해서 이어감 */
	void WarmUpBudgeted(ECPMonsterType Type, int32 TargetCount);

	/** WarmUpBudgeted()가 한 번 호출(한 프레임)에 실제로 SpawnActor할 최대 개수 */
	UPROPERTY(EditDefaultsOnly, Category = "Pool", meta = (ClampMin = 1))
	int32 WarmUpBatchSize = 20;

	UPROPERTY()
	TMap<ECPMonsterType, FCPMonsterPool> Pools;
};

// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Pool/CPMonsterPoolSubsystem.h"
#include "Monster/CPMonsterBase.h"
#include "Engine/World.h"

void UCPMonsterPoolSubsystem::WarmUp(ECPMonsterType Type, TSubclassOf<ACPMonsterBase> MonsterClass, int32 InCount)
{
	UWorld* World = GetWorld();
	if (!IsValid(MonsterClass) || InCount <= 0 || !World)
	{
		return;
	}

	FCPMonsterPool& Pool = Pools.FindOrAdd(Type);
	Pool.MonsterClass = MonsterClass;
	Pool.MaxPoolSize = FMath::Max(Pool.MaxPoolSize, InCount);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 비행형(Ranged/Bomb)은 BeginPlay에서 스폰 시점 위치를 PlaneConstraint 기준점으로 그대로 캐시함(원본
	// ACPMonsterRanged/ACPMonsterBomb::BeginPlay 참고) - 월드 원점(z=0)에 스폰하면 그 기준점이 지면 아래로
	// 잡혀서, 나중에 Acquire()로 재배치해도 시각적으로 파묻힌 것처럼 보일 수 있음. 최소한 CDO의
	// GetSpawnHeightOffset()만큼은 띄운 위치에서 BeginPlay가 돌게 해 기준점 자체를 정상 범위로 만들어둠
	FTransform WarmUpTransform = FTransform::Identity;
	if (ACPMonsterBase* MonsterCDO = MonsterClass->GetDefaultObject<ACPMonsterBase>())
	{
		WarmUpTransform.SetLocation(FVector(0.f, 0.f, MonsterCDO->GetSpawnHeightOffset()));
	}

	while (Pool.InactiveActors.Num() < InCount)
	{
		ACPMonsterBase* Monster = World->SpawnActor<ACPMonsterBase>(MonsterClass, WarmUpTransform, SpawnParams);
		if (!Monster)
		{
			break;
		}

		// 스폰 직후 AutoPossessAI가 자동으로 AI를 돌려버리므로(ACPMonsterAIController::OnPossess),
		// 미리 채워두는 액터도 반드시 이 함수로 꺼줘야 함
		Monster->OnReturnedToPool();
		Pool.InactiveActors.Add(Monster);
	}
}

ACPMonsterBase* UCPMonsterPoolSubsystem::Acquire(ECPMonsterType Type, TSubclassOf<ACPMonsterBase> MonsterClass, const FTransform& Transform)
{
	if (!IsValid(MonsterClass))
	{
		return nullptr;
	}

	FCPMonsterPool& Pool = Pools.FindOrAdd(Type);
	Pool.MonsterClass = MonsterClass;

	while (!Pool.InactiveActors.IsEmpty())
	{
		ACPMonsterBase* Monster = Pool.InactiveActors.Pop();
		if (!IsValid(Monster))
		{
			continue;
		}

		Monster->OnAcquiredFromPool(Transform);
		return Monster;
	}

	// 풀이 비어있음(WarmUp 예측을 초과하는 드문 경우) - 성능은 손해보더라도 정상 동작은 보장하기 위해 새로 스폰
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	return World->SpawnActor<ACPMonsterBase>(MonsterClass, Transform, SpawnParams);
}

void UCPMonsterPoolSubsystem::Release(ACPMonsterBase* Monster)
{
	if (!IsValid(Monster))
	{
		return;
	}

	FCPMonsterPool& Pool = Pools.FindOrAdd(Monster->GetMonsterType());
	if (!IsValid(Pool.MonsterClass))
	{
		Pool.MonsterClass = Monster->GetClass();
	}
	if (Pool.MaxPoolSize <= 0)
	{
		// WarmUp 없이 바로 죽은 예외 케이스 대비 - 최소 1마리 분량은 재사용 가능하게 함
		Pool.MaxPoolSize = 1;
	}

	if (Pool.InactiveActors.Num() >= Pool.MaxPoolSize)
	{
		Monster->Destroy();
		return;
	}

	Monster->OnReturnedToPool();
	Pool.InactiveActors.Add(Monster);
}

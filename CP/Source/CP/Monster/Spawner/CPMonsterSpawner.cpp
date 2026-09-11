// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Monster/CPMonsterBase.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

// Sets default values
ACPMonsterSpawner::ACPMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

FVector ACPMonsterSpawner::ResolveFreeSpawnLocation(const FVector& InDesiredLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return InDesiredLocation;
	}

	const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapCheckRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (!World->OverlapAnyTestByChannel(InDesiredLocation, FQuat::Identity, ECC_Pawn, ProbeShape, QueryParams))
	{
		return ProjectToNavMesh(InDesiredLocation); // 원래 위치가 비어있으면 그대로 사용(단, NavMesh 위로 보정)
	}

	// 이미 다른 몬스터/장애물이 있으면, 원래 위치 주변을 원형으로 훑어서 비어있는 자리를 찾음
	for (int32 Attempt = 1; Attempt <= MaxRelocationAttempts; ++Attempt)
	{
		const float AngleDeg = (360.f / MaxRelocationAttempts) * Attempt;
		const FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(AngleDeg)), FMath::Sin(FMath::DegreesToRadians(AngleDeg)), 0.f) * RelocationStepDistance;
		const FVector Candidate = InDesiredLocation + Offset;

		if (!World->OverlapAnyTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, ProbeShape, QueryParams))
		{
			return ProjectToNavMesh(Candidate);
		}
	}

	// 전부 막혀있으면 원래 위치를 그대로 반환 - SpawnActor의 AdjustIfPossibleButAlwaysSpawn이 최후 보정을 시도함
	return ProjectToNavMesh(InDesiredLocation);
}

FVector ACPMonsterSpawner::ProjectToNavMesh(const FVector& InLocation) const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavSys)
	{
		return InLocation;
	}

	FNavLocation OutNavLocation;
	constexpr float ProjectionExtentXY = 200.f;
	constexpr float ProjectionExtentZ = 200.f;
	if (NavSys->ProjectPointToNavigation(InLocation, OutNavLocation, FVector(ProjectionExtentXY, ProjectionExtentXY, ProjectionExtentZ)))
	{
		return OutNavLocation.Location;
	}

	// 투영 범위 안에 NavMesh가 전혀 없으면 보정할 방법이 없으므로 원래 위치를 그대로 반환
	return InLocation;
}

TArray<ACPMonsterBase*> ACPMonsterSpawner::SpawnMonsterRow(TSubclassOf<ACPMonsterBase> MonsterClass, int32 InCount, float InRowSpacingY, int32 InRound, int32 InWave)
{
	TArray<ACPMonsterBase*> SpawnedMonsters;

	if (!IsValid(MonsterClass) || InCount <= 0 || !GetWorld())
	{
		return SpawnedMonsters;
	}

	FTransform BaseTransform = SpawnCapsule->GetComponentTransform();
	const FVector RightAxis = FVector::YAxisVector; // 스포너 회전과 무관하게 항상 월드 Y축 기준으로 나란히 배치

	// 몬스터 클래스마다 캡슐 Half Height(또는 비행 몬스터의 고정 스폰 높이)가 달라서, SpawnCapsule의
	// 고정 Z(90)를 그대로 쓰면 살짝 떠서 스폰됐다가 떨어지거나 파묻히는 문제가 생김.
	// 스포너 액터 자체가 지면에 놓여있다고 가정하고, 그 위로 몬스터별 스폰 높이만큼만 띄움
	if (const ACPMonsterBase* MonsterCDO = MonsterClass->GetDefaultObject<ACPMonsterBase>())
	{
		FVector SpawnLocation = BaseTransform.GetLocation();
		SpawnLocation.Z = GetActorLocation().Z + MonsterCDO->GetSpawnHeightOffset();
		BaseTransform.SetLocation(SpawnLocation);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedMonsters.Reserve(InCount);

	for (int32 i = 0; i < InCount; ++i)
	{
		// (현재 인덱스 - 가운데 인덱스) 떨어진 만큼 옆으로
		const float Offset = (i - (InCount - 1) / 2.0f) * InRowSpacingY;

		FTransform SpawnTransform = BaseTransform;
		SpawnTransform.AddToTranslation(RightAxis * Offset);

		// 이 자리에 이미 다른 몬스터/장애물이 있으면 주변의 비어있는 자리로 대신 스폰함
		SpawnTransform.SetLocation(ResolveFreeSpawnLocation(SpawnTransform.GetLocation()));

		if (ACPMonsterBase* SpawnedMonster = GetWorld()->SpawnActor<ACPMonsterBase>(MonsterClass, SpawnTransform, SpawnParams))
		{
			SpawnedMonster->ApplyWaveStat(InRound, InWave);
			SpawnedMonsters.Add(SpawnedMonster);
		}
	}

	return SpawnedMonsters;
}

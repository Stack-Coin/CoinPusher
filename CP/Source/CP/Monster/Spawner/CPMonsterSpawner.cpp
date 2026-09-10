// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Monster/CPMonsterBase.h"
#include "Engine/World.h"

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

ACPMonsterBase* ACPMonsterSpawner::SpawnMonsterRow(TSubclassOf<ACPMonsterBase> MonsterClass, int32 InCount, float InRowSpacingY, int32 InWave)
{
	if (!IsValid(MonsterClass) || InCount <= 0 || !GetWorld())
	{
		return nullptr;
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

	ACPMonsterBase* LastSpawned = nullptr;

	for (int32 i = 0; i < InCount; ++i)
	{
		// (현재 인덱스 - 가운데 인덱스) 떨어진 만큼 옆으로
		const float Offset = (i - (InCount - 1) / 2.0f) * InRowSpacingY;

		FTransform SpawnTransform = BaseTransform;
		SpawnTransform.AddToTranslation(RightAxis * Offset);

		if (ACPMonsterBase* SpawnedMonster = GetWorld()->SpawnActor<ACPMonsterBase>(MonsterClass, SpawnTransform, SpawnParams))
		{
			SpawnedMonster->ApplyWaveStat(InWave);
			LastSpawned = SpawnedMonster;
		}
	}

	return LastSpawned;
}

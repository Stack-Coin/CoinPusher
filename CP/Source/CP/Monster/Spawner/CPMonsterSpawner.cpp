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

// Called when the game starts or when spawned
void ACPMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	CurrentWaveIndex = 0;
	StartWave();
}

void ACPMonsterSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(WaveTimer);
	GetWorld()->GetTimerManager().ClearTimer(RoundEndTimer);
}

void ACPMonsterSpawner::StartWave()
{
	// 정해진 웨이브(WaveCount)를 모두 마쳤다면, 기획서의 "라운드 대기시간"으로 넘어갑니다.
	if (CurrentWaveIndex >= WaveCount)
	{
		StartRoundEndWait();
		return;
	}

	CurrentPhase = ECPWavePhase::Spawning;
	BurstsSpawnedThisWave = 0;

	// 즉시 첫 무리를 스폰한 뒤, SpawnInterval 간격으로 BurstsPerWave번 반복 스폰합니다.
	// 매번 MonstersPerBurst마리가 한꺼번에 나오기 때문에 "군집(클러스터) 스폰"이 됩니다.
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ACPMonsterSpawner::SpawnBurst, SpawnInterval, true, 0.f);
}

void ACPMonsterSpawner::SpawnBurst()
{
	if (IsValid(EnemyClass))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// MonstersPerBurst마리를 같은 위치에서 한꺼번에 스폰합니다.
		// AdjustIfPossibleButAlwaysSpawn 덕분에 서로 겹치지 않도록 엔진이 알아서 살짝 밀어서 배치해줍니다.
		for (int32 i = 0; i < MonstersPerBurst; ++i)
		{
			GetWorld()->SpawnActor<ACPMonsterBase>(EnemyClass, SpawnCapsule->GetComponentTransform(), SpawnParams);
		}
	}

	++BurstsSpawnedThisWave;

	if (BurstsSpawnedThisWave >= BurstsPerWave)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
		EndWave();
	}
}

void ACPMonsterSpawner::EndWave()
{
	++CurrentWaveIndex;

	if (CurrentWaveIndex >= WaveCount)
	{
		// 마지막 일반 몬스터 웨이브까지 종료. 보스 웨이브는 아직 미구현이라, 라운드 대기시간으로 넘어갑니다.
		StartRoundEndWait();
		return;
	}

	CurrentPhase = ECPWavePhase::WaveWait;
	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &ACPMonsterSpawner::StartWave, WaveInterval, false);
}

void ACPMonsterSpawner::StartRoundEndWait()
{
	CurrentPhase = ECPWavePhase::RoundWait;
	GetWorld()->GetTimerManager().SetTimer(RoundEndTimer, this, &ACPMonsterSpawner::FinishRound, RoundEndWaitTime, false);
}

void ACPMonsterSpawner::FinishRound()
{
	CurrentPhase = ECPWavePhase::Finished;

	// 프로토타입: 라운드가 1개뿐이라 여기서 그냥 멈춥니다.
	// (추후 라운드가 여러 개가 되면, 여기서 다음 라운드를 시작하거나 보스 웨이브로 넘어가면 됩니다)
}

float ACPMonsterSpawner::GetWaveWaitSecondsRemaining() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(WaveTimer));
}

float ACPMonsterSpawner::GetRoundWaitSecondsRemaining() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(RoundEndTimer));
}

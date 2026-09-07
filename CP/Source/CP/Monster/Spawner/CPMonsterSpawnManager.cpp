// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawnManager.h"
#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Monster/CPMonsterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACPMonsterSpawnManager::ACPMonsterSpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACPMonsterSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	RegisterSpawners();

	CurrentWaveIndex = 0;
	StartWave(CurrentWaveIndex);
}

void ACPMonsterSpawnManager::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UWorld* World = GetWorld())
	{
		for (FCPActiveSpawnJob& Job : ActiveJobs)
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}

		World->GetTimerManager().ClearTimer(WaveWaitTimer);
		World->GetTimerManager().ClearTimer(RoundWaitTimer);
	}
}

void ACPMonsterSpawnManager::RegisterSpawners()
{
	SpawnersByIndex.Reset();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACPMonsterSpawner::StaticClass(), Found);

	static const FString Prefix = TEXT("Spawner");

	for (AActor* Actor : Found)
	{
		ACPMonsterSpawner* Spawner = Cast<ACPMonsterSpawner>(Actor);
		if (!Spawner)
		{
			continue;
		}

		for (const FName& Tag : Spawner->Tags)
		{
			const FString TagStr = Tag.ToString();
			if (TagStr.StartsWith(Prefix))
			{
				const FString IndexStr = TagStr.RightChop(Prefix.Len());
				if (IndexStr.IsNumeric())
				{
					SpawnersByIndex.Add(FCString::Atoi(*IndexStr), Spawner);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManager] Spawner tag '%s' on %s has no numeric index."), *TagStr, *Spawner->GetName());
				}
				break;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[CPMonsterSpawnManager] Registered %d spawner(s) by tag."), SpawnersByIndex.Num());
}

void ACPMonsterSpawnManager::GetWaveEntries(int32 InRound, int32 InWave, TArray<FCPSpawnWaveEntryRow*>& OutEntries) const
{
	OutEntries.Reset();

	if (!SpawnWaveEntryTable)
	{
		return;
	}

	TArray<FCPSpawnWaveEntryRow*> AllRows;
	SpawnWaveEntryTable->GetAllRows<FCPSpawnWaveEntryRow>(TEXT("ACPMonsterSpawnManager::GetWaveEntries"), AllRows);

	for (FCPSpawnWaveEntryRow* Row : AllRows)
	{
		if (Row && Row->Round == InRound && Row->Wave == InWave)
		{
			OutEntries.Add(Row);
		}
	}
}

FCPBossWaveRow* ACPMonsterSpawnManager::FindBossWaveRow(int32 InRound) const
{
	if (!BossWaveTable)
	{
		return nullptr;
	}

	TArray<FCPBossWaveRow*> AllRows;
	BossWaveTable->GetAllRows<FCPBossWaveRow>(TEXT("ACPMonsterSpawnManager::FindBossWaveRow"), AllRows);

	for (FCPBossWaveRow* Row : AllRows)
	{
		if (Row && Row->Round == InRound)
		{
			return Row;
		}
	}

	return nullptr;
}

void ACPMonsterSpawnManager::StartWave(int32 InWaveIndex)
{
	const int32 WaveNumber = InWaveIndex + 1; // 데이터 테이블의 Wave 컬럼은 1부터 시작

	TArray<FCPSpawnWaveEntryRow*> Entries;
	GetWaveEntries(CurrentRound, WaveNumber, Entries);

	if (Entries.IsEmpty())
	{
		// 이번 라운드에 더 이상 웨이브가 없으므로 보스 웨이브로 전환
		StartBossWave();
		return;
	}

	CurrentWaveIndex = InWaveIndex;
	CurrentPhase = ECPWavePhase::Spawning;

	ActiveJobs.Reset();
	WaveSpawnTotalSeconds = 0.f;
	WaveStartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	CurrentWaveEndWaitTime = Entries[0]->WaveEndWaitTime; // 같은 웨이브의 행들은 동일한 값을 넣는다고 가정

	for (const FCPSpawnWaveEntryRow* Entry : Entries)
	{
		FCPActiveSpawnJob Job;
		Job.MonsterClass = MonsterClassByType.FindRef(Entry->MonsterType);
		Job.CountPerSpawnPoint = Entry->CountPerSpawnPoint;
		Job.MonstersPerSpawn = Entry->MonstersPerSpawn;
		Job.SpawnRowSpacingY = Entry->SpawnRowSpacingY;

		for (int32 Index : Entry->SpawnerIndices)
		{
			if (ACPMonsterSpawner* Spawner = SpawnersByIndex.FindRef(Index))
			{
				Job.TargetSpawners.Add(Spawner);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManager] Spawner index %d not found (Round %d, Wave %d)."), Index, CurrentRound, WaveNumber);
			}
		}

		if (!IsValid(Job.MonsterClass) || Job.TargetSpawners.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawnManager] Skipping empty/invalid SpawnWaveEntryRow (Round %d, Wave %d)."), CurrentRound, WaveNumber);
			continue;
		}

		WaveSpawnTotalSeconds = FMath::Max(WaveSpawnTotalSeconds, Entry->SpawnInterval * Entry->CountPerSpawnPoint);

		const int32 JobIndex = ActiveJobs.Add(Job);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUFunction(this, FName("HandleSpawnJobTick"), JobIndex);
		GetWorld()->GetTimerManager().SetTimer(ActiveJobs[JobIndex].TimerHandle, TimerDelegate, Entry->SpawnInterval, true, 0.f);
	}

	// 이번 웨이브에 유효한 스폰 규칙이 하나도 없다면 곧바로 다음 단계로 진행
	if (ActiveJobs.IsEmpty())
	{
		EndWave();
	}
}

void ACPMonsterSpawnManager::HandleSpawnJobTick(int32 JobIndex)
{
	if (!ActiveJobs.IsValidIndex(JobIndex))
	{
		return;
	}

	FCPActiveSpawnJob& Job = ActiveJobs[JobIndex];

	for (ACPMonsterSpawner* Spawner : Job.TargetSpawners)
	{
		if (IsValid(Spawner))
		{
			// WaveStatTable의 Wave 필드와 매칭되는 값. 라운드 진행 방식이 정해지면 계산식을 조정하세요.
			Spawner->SpawnMonsterRow(Job.MonsterClass, Job.MonstersPerSpawn, Job.SpawnRowSpacingY, CurrentWaveIndex + 1);
		}
	}

	++Job.SpawnedCount;

	if (Job.SpawnedCount >= Job.CountPerSpawnPoint)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Job.TimerHandle);
		}

		CheckWaveComplete();
	}
}

void ACPMonsterSpawnManager::CheckWaveComplete()
{
	for (const FCPActiveSpawnJob& Job : ActiveJobs)
	{
		if (Job.SpawnedCount < Job.CountPerSpawnPoint)
		{
			return; // 아직 안 끝난 Job이 있음
		}
	}

	EndWave();
}

void ACPMonsterSpawnManager::EndWave()
{
	CurrentPhase = ECPWavePhase::WaveWait;

	GetWorld()->GetTimerManager().SetTimer(WaveWaitTimer, this, &ACPMonsterSpawnManager::HandleWaveWaitFinished, CurrentWaveEndWaitTime, false);
}

void ACPMonsterSpawnManager::HandleWaveWaitFinished()
{
	StartWave(CurrentWaveIndex + 1);
}

void ACPMonsterSpawnManager::StartBossWave()
{
	CurrentPhase = ECPWavePhase::RoundWait;

	const FCPBossWaveRow* BossRow = FindBossWaveRow(CurrentRound);
	const float WaitTime = BossRow ? BossRow->RoundEndWaitTime : 0.f;

	GetWorld()->GetTimerManager().SetTimer(RoundWaitTimer, this, &ACPMonsterSpawnManager::SpawnBoss, WaitTime, false);
}

void ACPMonsterSpawnManager::SpawnBoss()
{
	const FCPBossWaveRow* BossRow = FindBossWaveRow(CurrentRound);
	if (!BossRow)
	{
		UE_LOG(LogTemp, Error, TEXT("[CPMonsterSpawnManager] BossWaveTable has no row for Round %d."), CurrentRound);
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	ACPMonsterSpawner* BossSpawner = SpawnersByIndex.FindRef(BossRow->BossSpawnerIndex);
	if (!BossSpawner)
	{
		UE_LOG(LogTemp, Error, TEXT("[CPMonsterSpawnManager] Boss spawner index %d not found."), BossRow->BossSpawnerIndex);
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	TSubclassOf<ACPMonsterBase> BossClass = MonsterClassByType.FindRef(BossRow->BossMonsterType);
	if (!IsValid(BossClass))
	{
		UE_LOG(LogTemp, Error, TEXT("[CPMonsterSpawnManager] MonsterClassByType has no entry for BossMonsterType (%d)."), static_cast<int32>(BossRow->BossMonsterType));
		CurrentPhase = ECPWavePhase::Finished;
		return;
	}

	ACPMonsterBase* SpawnedBoss = BossSpawner->SpawnMonsterRow(BossClass, 1, 0.f, GetWaveCount());
	if (SpawnedBoss)
	{
		ActiveBoss = SpawnedBoss;
		SpawnedBoss->OnMonsterDied.AddUniqueDynamic(this, &ACPMonsterSpawnManager::HandleBossDied);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CPMonsterSpawnManager] Boss SpawnActor failed."));
		CurrentPhase = ECPWavePhase::Finished;
	}
}

void ACPMonsterSpawnManager::HandleBossDied()
{
	// 기획서: 보스 사망시 나머지 일반 몬스터도 즉시 제거
	TArray<AActor*> RemainingMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACPMonsterBase::StaticClass(), RemainingMonsters);

	for (AActor* Actor : RemainingMonsters)
	{
		if (IsValid(Actor) && Actor != ActiveBoss.Get())
		{
			Actor->Destroy();
		}
	}

	ActiveBoss = nullptr;
	CurrentPhase = ECPWavePhase::Finished;

	// 프로토타입: 라운드가 1개뿐이라 여기서 종료합니다.
	// 라운드가 여러 개가 되면: ++CurrentRound; CurrentWaveIndex = 0; StartWave(0); 를 여기서 호출하면 됩니다.
}

int32 ACPMonsterSpawnManager::GetWaveCount() const
{
	if (!SpawnWaveEntryTable)
	{
		return 0;
	}

	TArray<FCPSpawnWaveEntryRow*> AllRows;
	SpawnWaveEntryTable->GetAllRows<FCPSpawnWaveEntryRow>(TEXT("ACPMonsterSpawnManager::GetWaveCount"), AllRows);

	int32 MaxWave = 0;
	for (const FCPSpawnWaveEntryRow* Row : AllRows)
	{
		if (Row && Row->Round == CurrentRound)
		{
			MaxWave = FMath::Max(MaxWave, Row->Wave);
		}
	}

	return MaxWave;
}

int32 ACPMonsterSpawnManager::GetSpawnElapsedSeconds() const
{
	if (!GetWorld())
	{
		return 0;
	}

	const float Elapsed = FMath::Clamp(GetWorld()->GetTimeSeconds() - WaveStartWorldTime, 0.f, WaveSpawnTotalSeconds);
	return FMath::RoundToInt(Elapsed);
}

float ACPMonsterSpawnManager::GetRoundEndWaitSeconds() const
{
	const FCPBossWaveRow* BossRow = FindBossWaveRow(CurrentRound);
	return BossRow ? BossRow->RoundEndWaitTime : 0.f;
}

float ACPMonsterSpawnManager::GetWaveWaitSecondsRemaining() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(WaveWaitTimer));
}

float ACPMonsterSpawnManager::GetRoundWaitSecondsRemaining() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(RoundWaitTimer));
}

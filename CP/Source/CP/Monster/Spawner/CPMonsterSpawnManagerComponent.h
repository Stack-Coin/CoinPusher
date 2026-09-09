// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Monster/Spawner/CPMonsterSpawnTypes.h"
#include "CPMonsterSpawnManagerComponent.generated.h"

class UDataTable;
class ACPMonsterSpawner;
class ACPMonsterBase;

USTRUCT()
struct FCPActiveSpawnJob
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ACPMonsterSpawner>> TargetSpawners;

	UPROPERTY()
	TSubclassOf<ACPMonsterBase> MonsterClass;

	UPROPERTY()
	int32 CountPerSpawnPoint = 0;

	UPROPERTY()
	int32 SpawnedCount = 0;

	UPROPERTY()
	int32 MonstersPerSpawn = 1;

	UPROPERTY()
	float SpawnRowSpacingY = 100.f;

	UPROPERTY()
	FTimerHandle TimerHandle;
};

/*
BeginPlay()                                  ← 엔진이 호출
 ├─ LoadAsset()
 ├─ ApplyRoundInfo(CurrentRound)
 │    ├─ FindRoundInfoRow(InRound)            (조회만, 상태 변경 없음)
 │    └─ CreateSpawnerRing(...)               (기존 스포너 파괴 + 새로 생성을 한 함수 안에서 처리)
 └─ StartWave(0)
      ├─ GetWaveEntries(...)                  (조회만)
      ├─ ResolveValidSpawners(...)            (조회만)
      │    └─ IsSpawnerLocationValid(...)     (조회만)
      │
      ├─ [이번 웨이브 행이 없으면] StartBossWave()
      │                              └─ (대기 후) SpawnBoss()
      │                                            ├─ FindRoundInfoRow(...)
      │                                            ├─ IsSpawnerLocationValid(...)
      │                                            └─ [보스 죽으면] HandleBossDied()
      │
      └─ [행이 있으면] 타이머 등록
                        └─ (반복) HandleSpawnJobTick(JobIndex)
                                    └─ [이 Job이 끝나면, 다른 Job들도 확인해서] EndWave()
                                                    └─ (대기 후) HandleWaveWaitFinished()
                                                                   └─ StartWave(다음 웨이브)  ─┐
                                                                                                │
                                                    (여기서 다시 StartWave로 돌아가서 순환) ◄────┘

EndPlay()   ← 독립적. 남아있는 모든 타이머(Job들 + WaveWait + RoundWait) 정리만
*/

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CP_API UCPMonsterSpawnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCPMonsterSpawnManagerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	void LoadAsset();

	void ApplyRoundInfo(int32 InRound);
	void CreateSpawnerRing(int32 InSpawnerCount, float InSpawnerRadius);

	void StartWave(int32 InWaveIndex);
	void GetWaveEntries(int32 InRound, int32 InWave, TArray<FCPSpawnWaveEntryRow*>& OutEntries) const;
	void StartBossWave();
	TArray<TObjectPtr<ACPMonsterSpawner>> ResolveValidSpawners(const TArray<int32>& InIndices) const;

	bool IsSpawnerLocationValid(const FVector& InLocation) const;
	
	void EndWave();

	UFUNCTION()
	void SpawnBoss();

	UFUNCTION()
	void HandleWaveWaitFinished();

	UFUNCTION()
	void HandleSpawnJobTick(int32 JobIndex);

	UFUNCTION()
	void HandleBossDied();

	FCPRoundInfoRow* FindRoundInfoRow(int32 InRound) const;

public:
	// ----- 화면 표시(UI)용 getter - 기존 ACPMonsterSpawnManager가 갖고 있던 것과 동일한 이름/의미로 맞춤 -----
	ECPWavePhase GetCurrentPhase() const { return CurrentPhase; }
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }
	int32 GetWaveCount() const;

	// Spawning 단계: 이번 웨이브에서 지금까지 지난 스폰 시간 / 이번 웨이브의 전체 스폰 소요 시간(초, 추정치)
	int32 GetSpawnElapsedSeconds() const;
	int32 GetSpawnTotalSeconds() const { return FMath::RoundToInt(WaveSpawnTotalSeconds); }

	float GetWaveIntervalSeconds() const { return CurrentWaveEndWaitTime; }
	float GetRoundEndWaitSeconds() const;

	// WaveWait / RoundWait 단계: 남은 시간(초). 해당 단계가 아니면 0을 반환합니다.
	float GetWaveWaitSecondsRemaining() const;
	float GetRoundWaitSecondsRemaining() const;

protected:
	UPROPERTY(EditAnywhere, Category = "SpawnerRing")
	int32 SpawnerCount = 8;

	UPROPERTY(EditAnywhere, Category = "SpawnerRing", meta = (ClampMin = 0))
	float SpawnerRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "SpawnerRing")
	TSubclassOf<ACPMonsterSpawner> SpawnerClass;

	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> WaveInfoTable;

	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> RoundInfoTable;

	TMap<ECPMonsterType, TSubclassOf<ACPMonsterBase>> MonsterClassByType;

	UPROPERTY(EditAnywhere, Category = "Round")
	int32 CurrentRound = 1;

private:
	UPROPERTY()
	TMap<int32, TObjectPtr<ACPMonsterSpawner>> SpawnersByIndex;

	UPROPERTY()
	TArray<FCPActiveSpawnJob> ActiveJobs;

	UPROPERTY()
	TWeakObjectPtr<ACPMonsterBase> ActiveBoss;

	ECPWavePhase CurrentPhase = ECPWavePhase::Spawning;
	int32 CurrentWaveIndex = 0;

	float WaveSpawnTotalSeconds = 0.f;
	float WaveStartWorldTime = 0.f;
	float CurrentWaveEndWaitTime = 0.f;

	FTimerHandle WaveWaitTimer;
	FTimerHandle RoundWaitTimer;
};

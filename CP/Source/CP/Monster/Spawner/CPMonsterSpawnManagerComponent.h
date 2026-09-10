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
      │                                                          └─ 다음 라운드 있으면 ApplyRoundInfo(++CurrentRound)
      │                                                             → StartWave(0)로 순환, 없으면 Finished
      │
      └─ [행이 있으면] 타이머 등록 (마지막 웨이브라면 등록 직후 곧바로 StartBossWave()도 호출)
                        └─ (반복) HandleSpawnJobTick(JobIndex)
                                    │   (스폰될 때마다 WaveAliveMonsterCount++,
                                    │    그 몬스터의 OnMonsterDied를 HandleWaveMonsterDied에 구독)
                                    └─ [이 Job이 끝나면, 다른 Job들도 확인해서]
                                          ├─ [이번이 라운드의 마지막 웨이브가 아니면] EndWave()
                                          │                 └─ (대기 후) HandleWaveWaitFinished()
                                          │                                └─ StartWave(다음 웨이브) ─┐
                                          │                                                            │
                                          │                 (여기서 다시 StartWave로 돌아가서 순환) ◄───┘
                                          │
                                          └─ [마지막 웨이브] StartWave() 시작 시점에 곧바로 StartBossWave()가
                                                        이미 호출되어 있음 - 전멸 대기 없이 마지막 웨이브 몹과
                                                        보스가 같은 페이즈에 함께 등장함 (여기서는 할 일 없음)

EndPlay()   ← 독립적. 남아있는 모든 타이머(Job들 + WaveWait + RoundWait) 정리만

* 웨이브끼리(Wave→Wave)는 시간(WaveEndWaitTime) 기반, 마지막 웨이브→보스는 시간(RoundEndWaitTime) 기반 -
  둘 다 몬스터 전멸과 무관한 타이머 트리거임. WaveAliveMonsterCount는 라운드 시작(ApplyRoundInfo)에서
  0으로 리셋되고 그 라운드의 모든 웨이브에 걸쳐 누적으로 세고 줄어들지만, 지금은 화면 표시 등 참고용일
  뿐 어떤 전환도 트리거하지 않음.
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
	void GetWaveEntries(int32 InRound, int32 InWave, TArray<FCPMonsterWaveInfoRow*>& OutEntries) const;
	void StartBossWave();
	TArray<TObjectPtr<ACPMonsterSpawner>> ResolveValidSpawners(const TArray<int32>& InIndices) const;

	bool IsSpawnerLocationValid(const FVector& InLocation) const;

	/** 지금 웨이브의 모든 ActiveJobs가 목표 마릿수까지 스폰을 끝냈는지 (스폰 진행 중이면 false) */
	bool IsWaveSpawningComplete() const;

	void EndWave();

	UFUNCTION()
	void SpawnBoss();

	UFUNCTION()
	void HandleWaveWaitFinished();

	UFUNCTION()
	void HandleSpawnJobTick(int32 JobIndex);

	UFUNCTION()
	void HandleBossDied();

	/** 웨이브에서 스폰된(보스 제외) 몬스터가 죽을 때마다 호출됨 - 화면 표시용 WaveAliveMonsterCount만
	 *  줄임. 보스 등장은 전멸 여부와 무관하게 타이머로만 진행되므로 여기서 다른 전환은 하지 않음 */
	UFUNCTION()
	void HandleWaveMonsterDied();

	FCPMonsterRoundInfoRow* FindRoundInfoRow(int32 InRound) const;

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

	/** 이번 라운드에서 (웨이브들에 걸쳐 누적으로) 스폰됐지만 아직 안 죽은 몬스터 수 - 라운드 시작
	 *  (ApplyRoundInfo)에서 0으로 리셋됨. 화면 표시 등 참고용 카운트이며, 더 이상 보스 등장 조건으로는
	 *  쓰이지 않음(보스는 마지막 웨이브 시작과 동시에 타이머로 등장함) */
	int32 WaveAliveMonsterCount = 0;

	/** StartWave()에서 계산: 다음 Wave 엔트리가 없어서(GetWaveEntries가 비어서) 이번이 이 라운드의
	 *  마지막 웨이브인지. true면 전멸을 기다리지 않고 StartWave() 안에서 곧바로 StartBossWave()도
	 *  호출되어, 마지막 웨이브 몹과 보스가 같은 페이즈에 함께 등장함 */
	bool bFinalWaveOfRound = false;

	float WaveSpawnTotalSeconds = 0.f;
	float WaveStartWorldTime = 0.f;
	float CurrentWaveEndWaitTime = 0.f;

	FTimerHandle WaveWaitTimer;
	FTimerHandle RoundWaitTimer;
};

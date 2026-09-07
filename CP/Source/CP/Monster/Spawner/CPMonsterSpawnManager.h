// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Monster/Spawner/CPMonsterSpawnTypes.h"
#include "CPMonsterSpawnManager.generated.h"

class UDataTable;
class ACPMonsterSpawner;
class ACPMonsterBase;

/** 매니저 내부에서 진행 중인 스폰 규칙 1개의 런타임 상태 (FCPSpawnWaveEntryRow 1행에 대응) */
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

/**
 * 몬스터 웨이브/라운드 진행을 전담하는 매니저. 레벨에 1개 배치합니다.
 *
 * SpawnWaveEntryTable(Round/Wave 컬럼으로 필터링하는 평탄한 테이블)에서 현재 라운드+웨이브에
 * 해당하는 행들을 찾아 진행하고, 각 행의 SpawnerIndices를 레벨의 ACPMonsterSpawner 태그
 * ("Spawner0", "Spawner1"...)와 매칭해 실제 스폰을 지시합니다.
 * 현재 라운드에 다음 웨이브 번호의 행이 하나도 없으면 마지막 웨이브로 간주하고,
 * BossWaveTable에서 같은 Round 행을 찾아 보스를 스폰합니다. 보스가 죽으면 라운드를 종료합니다.
 *
 * ACPMonsterSpawner는 이 매니저의 호출을 받아 실제로 SpawnActor만 수행하는 얇은 스폰 포인트입니다.
 */
UCLASS()
class CP_API ACPMonsterSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	ACPMonsterSpawnManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	/** 레벨의 모든 ACPMonsterSpawner를 찾아 Tag("SpawnerN")에서 인덱스를 파싱해 SpawnersByIndex에 등록 */
	void RegisterSpawners();

	/** InWaveIndex(0부터)번째 웨이브를 시작. SpawnWaveEntryTable에 Round=CurrentRound, Wave=InWaveIndex+1인
	 *  행이 하나도 없으면 이번 라운드의 웨이브가 끝난 것으로 보고 보스 웨이브로 전환 */
	void StartWave(int32 InWaveIndex);

	/** 이번 웨이브의 모든 Job이 끝났는지 확인하고, 끝났으면 EndWave 호출 */
	void CheckWaveComplete();

	/** 웨이브 종료 처리: WaveEndWaitTime 후 다음 웨이브 시작 */
	void EndWave();

	UFUNCTION()
	void HandleWaveWaitFinished();

	/** 라운드 대기(RoundEndWaitTime) 시작 - 마지막 웨이브가 끝났을 때 호출됨 */
	void StartBossWave();

	UFUNCTION()
	void SpawnBoss();

	/** 스폰 타이머 콜백. JobIndex번째 Job의 대상 스포너들에게 스폰을 지시 */
	UFUNCTION()
	void HandleSpawnJobTick(int32 JobIndex);

	/** 보스의 OnMonsterDied에 바인딩됨. 남은 잡몹을 정리하고 라운드를 종료 처리 */
	UFUNCTION()
	void HandleBossDied();

	/** SpawnWaveEntryTable에서 Round==InRound && Wave==InWave인 행을 전부 찾아 반환 */
	void GetWaveEntries(int32 InRound, int32 InWave, TArray<FCPSpawnWaveEntryRow*>& OutEntries) const;

	/** BossWaveTable에서 Round==InRound인 행을 찾아 반환 (없으면 nullptr) */
	FCPBossWaveRow* FindBossWaveRow(int32 InRound) const;

public:
	// ----- 화면 표시(UI)용 getter - 기존 ACPMonsterSpawner가 갖고 있던 것과 동일한 이름/의미로 맞춤 -----
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
	/** 웨이브별 스폰 규칙 (RowStructure = FCPSpawnWaveEntryRow). 엑셀/CSV로 관리하기 쉬운 평탄한 테이블입니다. */
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> SpawnWaveEntryTable;

	/** 라운드별 보스 설정 (RowStructure = FCPBossWaveRow) */
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> BossWaveTable;

	/** MonsterType별로 실제 스폰할 블루프린트 클래스 (BP_MonsterNormal/Tanker/Ranged/Boss 등).
	 *  BaseStatTable/WaveStatTable엔 "타입→스탯" 매핑만 있어서, "타입→스폰 클래스" 매핑은 매니저가 별도로 들고 있어야 합니다. */
	UPROPERTY(EditAnywhere, Category = "Data")
	TMap<ECPMonsterType, TSubclassOf<ACPMonsterBase>> MonsterClassByType;

	/** 시작 라운드 번호. 각 테이블에서 Round == CurrentRound인 행을 찾는 데 사용됩니다. */
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

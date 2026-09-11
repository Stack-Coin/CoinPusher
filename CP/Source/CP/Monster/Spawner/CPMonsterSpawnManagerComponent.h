// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Monster/Spawner/CPMonsterSpawnTypes.h"
#include "CPMonsterSpawnManagerComponent.generated.h"

class UDataTable;
class ACPMonsterSpawner;
class ACPMonsterBase;
class ACPCoinPusher;

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
용어: "마지막 웨이브" = WaveInfo에서 Wave 번호가 가장 큰 행 = 보스와 기본 몬스터가 동시에 등장하는 웨이브.
      "마지막 웨이브 직전 웨이브" = 그 바로 앞 웨이브 = 이 웨이브가 전멸해야 보스 페이즈로 넘어감.

BeginPlay() → LoadAsset() → ApplyRoundInfo(CurrentRound) → StartWave(0)

StartWave(N)                 웨이브 스폰 시작
 ├─ 이 웨이브가 마지막 웨이브 직전 웨이브(bIsWaveBeforeLastWave)면, 전멸(WaveAliveMonsterCount==0) 시 BeginRoundWait()
 └─ 아니면 전멸 시 EndWave() → (대기) → StartWave(N+1)

BeginRoundWait()              마지막 웨이브 직전 웨이브 전멸 확인 시 1회 호출
 └─ (RoundEndWaitTime 후) BeginBossPhase()
                             ├─ StartRoundMobSpawning()  - WaveInfo 마지막 행을 재사용해 잡몹 계속 스폰
                             └─ SpawnBoss()
                                   └─ [보스 사망] HandleBossDied()
                                         ├─ StopRoundMobSpawning()
                                         └─ 다음 라운드 있으면 (NextRoundStartDelay 후) StartNextRound(), 없으면 Finished

* 웨이브↔웨이브는 WaveEndWaitTime 기반 시간 전환, 마지막 스폰 웨이브→보스는 전멸 기반 전환.
  RoundMob과 보스는 항상 BeginBossPhase()에서 동시에 등장함(시차 없음).
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

	/** 마지막 웨이브 직전 웨이브 전멸 확인 후 호출 - RoundEndWaitTime 대기 후 BeginBossPhase() 실행 */
	void BeginRoundWait();

	TArray<TObjectPtr<ACPMonsterSpawner>> ResolveValidSpawners(const TArray<int32>& InIndices) const;

	/** NavMesh 위의 유효한 위치인지 확인. OutProjectedLocation을 넘기면 NavMesh가 계산해준 보정 위치를
	 *  같이 받아올 수 있음(스포너 링 생성 시 위치를 실제로 스냅시키는 데 사용 - CreateSpawnerRing 참고) */
	bool IsSpawnerLocationValid(const FVector& InLocation, FVector* OutProjectedLocation = nullptr) const;

	/** 지금 웨이브의 모든 ActiveJobs가 목표 마릿수까지 스폰을 끝냈는지 */
	bool IsWaveSpawningComplete() const;

	void EndWave();

	/** WaveInfo 마지막 행을 재사용해 보스 페이즈 내내 잡몹을 스폰함 (Boss 타입 행은 제외) */
	void StartRoundMobSpawning();
	void StopRoundMobSpawning();

	UFUNCTION()
	void SpawnBoss();

	/** RoundEndWaitTime 경과 후 호출 - RoundMob과 보스를 항상 동시에 등장시킴 */
	UFUNCTION()
	void BeginBossPhase();

	UFUNCTION()
	void HandleWaveWaitFinished();

	UFUNCTION()
	void HandleSpawnJobTick(int32 JobIndex);

	UFUNCTION()
	void HandleRoundMobSpawnTick(int32 JobIndex);

	UFUNCTION()
	void HandleBossDied();

	/** HandleBossDied()에서 NextRoundStartDelay 후 호출 - 다음 라운드로 전환 */
	UFUNCTION()
	void StartNextRound();

	/** 웨이브 몹(보스 제외)이 죽을 때마다 호출 - 전멸 시 BeginRoundWait()를 트리거함 */
	UFUNCTION()
	void HandleWaveMonsterDied();

	/** 스폰된 모든 몬스터(웨이브 몹/RoundMob/보스 전부)가 죽을 때마다 호출 - TotalAliveMonsterCount만 줄임.
	 *  MaxAliveMonsterCount 상한 체크용으로, 위 HandleWaveMonsterDied/HandleBossDied와 별개로 항상 같이 구독됨 */
	UFUNCTION()
	void HandleAnyMonsterDied();

	FCPMonsterRoundInfoRow* FindRoundInfoRow(int32 InRound) const;

	/** 현재 라운드의 RoundInfo에서 MaxAliveMonsterCount를 읽어옴 (행이 없으면 0=무제한) */
	int32 GetMaxAliveMonsterCount() const;

	/** GetOwner()(=Player)에서 연결된 CoinPusher를 가져옴 (없으면 nullptr) - Boss/Bomb의 CoinPusher 연동
	 *  핸들러들과 DropZone 델리게이트 바인딩(BeginPlay)이 공통으로 사용 */
	ACPCoinPusher* GetCoinPusher() const;

	/** ACPMonsterBoss::OnBossAttackedPlayer에 바인딩됨(SpawnBoss) - 보스 공격이 플레이어에게 명중할 때마다
	 *  CoinPusher의 활성 코인을 몬스터 코인으로 전환시킴 */
	UFUNCTION()
	void HandleBossAttackedPlayer();

	/** ACPMonsterBomb::OnBombExplodedOnPlayer에 바인딩됨(스폰 시점마다) - 자폭 몬스터가 플레이어에 닿아
	 *  터질 때마다 CoinPusher에 몬스터 코인을 스폰함 */
	UFUNCTION()
	void HandleBombExplodedOnPlayer();

	/** CoinPusher->GetDropZoneDroppedDelegate()에 바인딩됨(BeginPlay) - DropZone에 몬스터 코인(ItemID ==
	 *  MonsterCoinItemID)이 떨어질 때마다 SpawnRandomRewardMonster()를 호출함. OnDropped는 떨어진
	 *  아이템 1개당 한 번씩 Broadcast되므로(Count 파라미터가 없음) 이벤트 1번 = 1개로 취급함 */
	UFUNCTION()
	void HandleDropZoneItemDropped(FName ItemID);

	/** 보스를 제외한 타입 중 하나를 무작위로 골라, 무작위 스포너 위치에 1마리 스폰 - DropZone 몬스터 코인
	 *  보상용. 보스처럼 MaxAliveMonsterCount 상한과 무관하게 항상 스폰됨(전멸 판정에도 관여하지 않음) */
	void SpawnRandomRewardMonster();

public:
	// ----- UI 표시용 getter -----
	ECPWavePhase GetCurrentPhase() const { return CurrentPhase; }
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }
	int32 GetWaveCount() const;

	// Spawning 단계: 지난 스폰 시간 / 이번 웨이브 전체 스폰 소요 시간(초, 추정치)
	int32 GetSpawnElapsedSeconds() const;
	int32 GetSpawnTotalSeconds() const { return FMath::RoundToInt(WaveSpawnTotalSeconds); }

	float GetWaveIntervalSeconds() const { return CurrentWaveEndWaitTime; }
	float GetRoundEndWaitSeconds() const;

	// WaveWait / RoundWait 단계 남은 시간(초). 해당 단계가 아니면 0
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

	/** CoinPusher의 ItemDataTable에 CoinType=Monster로 등록되어 있어야 하는 몬스터 코인 ItemID.
	 *  HandleBossAttackedPlayer/HandleBombExplodedOnPlayer가 CoinPusher를 호출할 때, 그리고
	 *  HandleDropZoneItemDropped가 DropZone에서 어떤 ItemID를 몬스터 코인으로 취급할지 판단할 때 사용 */
	UPROPERTY(EditAnywhere, Category = "CoinPusher Rewards")
	FName MonsterCoinItemID = FName("5C");

	/** 보스 공격이 플레이어에게 명중했을 때 CoinPusher->MonsterConvertActive()에 넘길 개수 */
	UPROPERTY(EditAnywhere, Category = "CoinPusher Rewards", meta = (ClampMin = 1))
	int32 MonsterConvertCountOnBossAttack = 1;

	/** 자폭 몬스터가 플레이어에 닿아 터졌을 때 CoinPusher->SpawnMonsterCoin()에 넘길 개수 */
	UPROPERTY(EditAnywhere, Category = "CoinPusher Rewards", meta = (ClampMin = 1))
	int32 MonsterCoinSpawnCountOnBombExplode = 1;

private:
	UPROPERTY()
	TMap<int32, TObjectPtr<ACPMonsterSpawner>> SpawnersByIndex;

	UPROPERTY()
	TArray<FCPActiveSpawnJob> ActiveJobs;

	/** 보스 페이즈 동안 WaveInfo 마지막 행을 재사용해 스폰하는 Job들 (ActiveJobs와 별개) */
	UPROPERTY()
	TArray<FCPActiveSpawnJob> RoundMobJobs;

	UPROPERTY()
	TWeakObjectPtr<ACPMonsterBase> ActiveBoss;

	ECPWavePhase CurrentPhase = ECPWavePhase::Spawning;
	int32 CurrentWaveIndex = 0;

	/** 이번 라운드 누적 생존 몬스터 수. ApplyRoundInfo()에서 0으로 리셋되고, 0이 되는 시점(+스폰 완료)이
	 *  마지막 스폰 웨이브의 전멸 판정 - BeginRoundWait() 호출 트리거 */
	int32 WaveAliveMonsterCount = 0;

	/** StartWave()에서 계산 - 이번이 "마지막 웨이브 직전 웨이브"인지 (마지막 웨이브는 WaveInfo의 진짜
	 *  마지막 행으로, 정상 스폰 대상이 아니라 보스와 동시 등장하는 RoundMob 전용으로 예약되어 있음).
	 *  true면 전멸 시 BeginRoundWait()가 호출됨 */
	bool bIsWaveBeforeLastWave = false;

	float WaveSpawnTotalSeconds = 0.f;
	float WaveStartWorldTime = 0.f;
	float CurrentWaveEndWaitTime = 0.f;

	/** 현재 월드에 살아있는 몬스터(웨이브 몹+RoundMob+보스) 총 수 - MaxAliveMonsterCount 상한 체크용.
	 *  WaveAliveMonsterCount(웨이브 전멸 판정용, 보스/RoundMob 미포함)와는 별개로 관리됨 */
	int32 TotalAliveMonsterCount = 0;

	FTimerHandle WaveWaitTimer;

	/** BeginRoundWait()(보스 등장 대기)와 HandleBossDied()(다음 라운드 대기) 양쪽에서 재사용됨 */
	FTimerHandle RoundWaitTimer;
};

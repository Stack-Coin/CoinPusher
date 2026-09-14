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
class UCPInGameWidget;

USTRUCT()
struct FCPActiveSpawnJob
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ACPMonsterSpawner>> TargetSpawners;

	UPROPERTY()
	TSubclassOf<ACPMonsterBase> MonsterClass;

	/** MonsterClass의 CDO에서 재추론하지 않고, WaveInfoTable의 이 Job을 만든 행이 갖고 있던 값을 그대로
	 *  저장해둠 - 몬스터 풀(UCPMonsterPoolSubsystem)이 타입별로 정확히 나뉘도록 스포너에 그대로 전달됨 */
	UPROPERTY()
	ECPMonsterType MonsterType = ECPMonsterType::Normal;

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

	/** 이번 "라운드"(TargetSpawners 전체에 MonstersPerSpawn씩 스폰하는 한 바퀴) 안에서 어디까지
	 *  처리했는지 - MaxMonstersPerJobTick 예산을 넘는 라운드를 여러 틱(SpawnInterval마다)에 걸쳐
	 *  나눠서 처리하기 위한 커서. 라운드를 끝까지 돌면 0으로 리셋되고 SpawnedCount가 증가함(한 프레임에
	 *  스포너 수 x MonstersPerSpawn만큼 몰아서 스폰해 프레임 히치가 나는 걸 막기 위함) */
	UPROPERTY()
	int32 NextSpawnerCursor = 0;
};

/*
용어: "마지막 웨이브" = WaveInfo에서 Wave 번호가 가장 큰 행 = 보스와 기본 몬스터가 동시에 등장하는 웨이브.
      "마지막 웨이브 직전 웨이브" = 그 바로 앞 웨이브 = 이 웨이브가 전멸해야 보스 페이즈로 넘어감.

BeginPlay() → ApplyRoundInfo(CurrentRound) → StartWave(0)

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

	void ApplyRoundInfo(int32 InRound);
	void CreateSpawnerRing(int32 InSpawnerCount, float InSpawnerRadius);

	/** 이 라운드의 WaveInfoTable 행들(마지막 웨이브=RoundMob 전용 행 포함)을 훑어서, 타입별로 동시에
	 *  살아있을 수 있는 최대 마릿수를 추정함(같은 웨이브 안의 CountPerSpawnPoint*스포너수 - 웨이브는
	 *  순차 진행이라 합산이 아니라 최댓값). 보스 타입은 RoundInfoTable 기준으로 항상 1을 보장.
	 *  UCPMonsterPoolSubsystem::WarmUp() 호출 크기를 정하는 데만 쓰임(성능 힌트일 뿐이라 추정이 어긋나도
	 *  Acquire()가 새로 스폰해서 동작은 정상적으로 유지됨) */
	void WarmUpMonsterPools(int32 InRound, const FCPMonsterRoundInfoRow* InRoundInfo) const;

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

	/** 보상 몬스터(SpawnRandomRewardMonster)가 죽을 때마다 호출 - ActiveRewardMonsterCount만 줄임.
	 *  MaxRewardMonsterCount 상한 체크용으로, HandleAnyMonsterDied와 별개로 보상 몬스터에만 추가로 구독됨 */
	UFUNCTION()
	void HandleRewardMonsterDied();

	FCPMonsterRoundInfoRow* FindRoundInfoRow(int32 InRound) const;

	/** 현재 라운드의 RoundInfo에서 MaxAliveMonsterCount를 읽어옴 (행이 없으면 0=무제한) */
	int32 GetMaxAliveMonsterCount() const;

	/** GetOwner()(=Player)에서 연결된 CoinPusher를 가져옴 (없으면 nullptr) - Boss/Bomb의 CoinPusher 연동
	 *  핸들러들과 DropZone 델리게이트 바인딩(BeginPlay)이 공통으로 사용 */
	ACPCoinPusher* GetCoinPusher() const;

	/** GetOwner()(=Player)의 현재 위치. 스폰 위치가 플레이어와 너무 가까워지는 걸 막는 데 씀
	 *  (ACPMonsterSpawner::ResolveFreeSpawnLocation의 MinPlayerSpawnDistance 판정) */
	FVector GetPlayerLocation() const;

	/** ACPMonsterBoss::OnBossAttackedPlayer에 바인딩됨(SpawnBoss) - 보스 공격이 플레이어에게 명중할 때마다
	 *  CoinPusher의 활성 코인을 몬스터 코인으로 전환시킴 */
	UFUNCTION()
	void HandleBossAttackedPlayer();

	/** UCPMonsterStatComponent::OnMonsterHealthChanged에 바인딩됨(SpawnBoss) - 보스가 데미지를 받을
	 *  때마다 InGameUI의 보스 체력 게이지를 갱신함 */
	UFUNCTION()
	void HandleBossHealthChanged(float CurrentHealth, float MaxHealth);

	/** GetOwner()(Player)의 컨트롤러(ACPTopDownPlayerController)에서 InGameUI를 가져옴 - 없으면 nullptr.
	 *  GetCoinPusher()와 같은 이유로 Player를 거쳐야 해서 별도 헬퍼로 분리함 */
	UCPInGameWidget* GetInGameWidget() const;

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
	/** MonsterClassByType 기본값을 하드코딩 경로로 채움 - 생성자에서만 호출됨(CDO/인스턴스 생성 시점).
	 *  BeginPlay가 아니라 생성자에서 실행돼야 쿠커가 CDO를 만들며 이 코드를 실행해 하드레퍼런스로
	 *  잡아줌(패키지 빌드에 포함됨) */
	void LoadDefaultMonsterClasses();

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

	/** 스폰 Job 하나가 한 틱(SpawnInterval마다)당 최대 몇 마리까지 스폰할지. 웨이브 데이터
	 *  (CountPerSpawnPoint/MonstersPerSpawn x 스포너 수)가 이보다 크면 한 틱에 몰아서 스폰하지 않고
	 *  여러 틱에 걸쳐 나눠서 처리함(FCPActiveSpawnJob::NextSpawnerCursor 참고) - 한 웨이브에 수백
	 *  마리를 스폰하는 기획에서 프레임 히치를 막기 위함 */
	UPROPERTY(EditAnywhere, Category = "Performance", meta = (ClampMin = 1))
	int32 MaxMonstersPerJobTick = 30;

	/** 몬스터 타입별 스폰 클래스. 생성자(LoadDefaultMonsterClasses)에서 하드코딩 경로로 기본값을 채움 -
	 *  BP Class Defaults에서 개별 타입만 다른 클래스로 덮어쓰고 싶으면 여기서 직접 지정해도 됨(생성자는
	 *  이미 값 있는 타입은 안 건드림). BP_CPPlayerCharacter Class Defaults에 이 맵을 직접 채워두면
	 *  안 됨 - 그러면 BP_CPPlayerCharacter 로드 시점에 몬스터 BP(+거기 딸린 메시/이펙트)가 전부 강제
	 *  로드돼서 무거운 나이아가라 VFX 때문에 에디터 로드가 오래 걸리거나 멈춘 것처럼 보임 */
	UPROPERTY(EditAnywhere, Category = "Data")
	TMap<ECPMonsterType, TSubclassOf<ACPMonsterBase>> MonsterClassByType;
	//TMap<ECPMonsterType, TSoftObjectPtr<ACPMonsterBase>> MonsterClassByType;

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

	// TODO. RoundInfo 넣기
	/** 보상 몬스터(SpawnRandomRewardMonster) 동시 생존 상한 - MaxAliveMonsterCount와는 별개로 관리됨.
	 *  0이면 무제한(GetMaxAliveMonsterCount()와 같은 컨벤션) */
	UPROPERTY(EditAnywhere, Category = "CoinPusher Rewards", meta = (ClampMin = 0))
	int32 MaxRewardMonsterCount = 99;

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

	/** 현재 살아있는 보상 몬스터(SpawnRandomRewardMonster) 수 - MaxRewardMonsterCount 상한 체크용 */
	int32 ActiveRewardMonsterCount = 0;

	FTimerHandle WaveWaitTimer;

	/** BeginRoundWait()(보스 등장 대기)와 HandleBossDied()(다음 라운드 대기) 양쪽에서 재사용됨 */
	FTimerHandle RoundWaitTimer;
};

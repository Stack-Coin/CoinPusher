// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPMonsterSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class ACPMonsterBase;
class ACPMonsterBoss;

/** 화면 표시(UCPUserWidget_WaveStatus)에서 쓰는, 스포너가 지금 어떤 단계에 있는지 나타내는 상태 */
UENUM(BlueprintType)
enum class ECPWavePhase : uint8
{
	Spawning     UMETA(DisplayName = "Spawning"),      // 몬스터 무리를 생성하는 중
	WaveWait     UMETA(DisplayName = "Wave Wait"),      // 웨이브 사이 대기 중
	RoundWait    UMETA(DisplayName = "Round Wait"),     // 마지막 웨이브 종료 후 라운드 대기 중
	Finished     UMETA(DisplayName = "Finished")        // 모든 웨이브 + 라운드 대기까지 종료
};

/**
 * 프로토타입용 몬스터 스포너.
 * 레벨에 여러 개 배치해서 각각을 "스폰 포인트"로 사용합니다.
 * 매 SpawnInterval마다 몬스터를 한 마리씩이 아니라 MonstersPerBurst마리를 한꺼번에("군집") 스폰합니다.
 * 리그 오브 레전드 미니언처럼 한 마리씩 줄지어 나오는 방식이 아니라, 이 스폰 포인트에서 여러 마리가
 * 동시에 무리지어 튀어나오는 "군집(클러스터) 스폰" 형태가 됩니다.
 * BurstsPerWave번 무리를 생성하면 한 웨이브가 끝나고, WaveInterval만큼 쉬었다가 다음 웨이브를 시작합니다.
 * WaveCount번째 웨이브까지 끝나면(=일반 몬스터 웨이브 종료), 기획서의 "라운드 대기시간"에 맞춰
 * RoundEndWaitTime만큼 더 대기한 뒤 멈춥니다. (보스 웨이브는 아직 미구현이라 그 이후는 진행하지 않습니다)
 */
UCLASS()
class CP_API ACPMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACPMonsterSpawner();

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	// 웨이브 시작: 무리(Burst) 카운트를 초기화하고 반복 스폰 타이머를 시작합니다.
	void StartWave();

	// 몬스터를 MonstersPerBurst마리 한꺼번에 생성(=군집 스폰). 이번 웨이브의 목표 무리 수를 채우면 웨이브를 종료합니다.
	UFUNCTION()
	void SpawnBurst();

	// 웨이브 종료 처리: 다음 웨이브가 남아있으면 WaveInterval 후 다음 웨이브를 시작하고,
	// 마지막 웨이브였다면 라운드 대기(RoundEndWaitTime)를 시작합니다.
	void EndWave();

	// 기획서의 "라운드 대기시간"에 해당하는 대기를 시작합니다.
	void StartRoundEndWait();

	// 라운드 대기가 끝났을 때 호출됩니다. 프로토타입은 라운드가 1개뿐이라 여기서 그냥 멈춥니다.
	UFUNCTION()
	void FinishRound();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;

	// ----- 화면 표시(UI)용 getter -----
	ECPWavePhase GetCurrentPhase() const { return CurrentPhase; }
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }
	int32 GetWaveCount() const { return WaveCount; }

	// Spawning 단계: 이번 웨이브에서 지금까지 지난 스폰 시간 / 이번 웨이브의 전체 스폰 소요 시간(초)
	int32 GetSpawnElapsedSeconds() const { return FMath::RoundToInt(BurstsSpawnedThisWave * SpawnInterval); }
	int32 GetSpawnTotalSeconds() const { return FMath::RoundToInt(BurstsPerWave * SpawnInterval); }

	float GetWaveIntervalSeconds() const { return WaveInterval; }
	float GetRoundEndWaitSeconds() const { return RoundEndWaitTime; }

	// WaveWait / RoundWait 단계: 남은 시간(초). 해당 단계가 아니면 0을 반환합니다.
	float GetWaveWaitSecondsRemaining() const;
	float GetRoundWaitSecondsRemaining() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TSubclassOf<ACPMonsterBase> EnemyClass;

	// 모든 일반 몬스터 웨이브(WaveCount) + 라운드 대기시간(RoundEndWaitTime)이 끝난 뒤 소환할 보스 몬스터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TSubclassOf<ACPMonsterBase> BossClass;

	// 프로토타입: 라운드당 일반 몬스터 웨이브 수 (보스 웨이브는 아직 미구현이라 제외)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 1))
	int32 WaveCount = 5;

	// 한 번에(=동시에) 스폰되는 몬스터 수. 이 값이 "군집"의 크기입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 1))
	int32 MonstersPerBurst = 3;

	// 한 웨이브 동안 이 스폰 포인트가 몬스터 무리(Burst)를 생성하는 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 1))
	int32 BurstsPerWave = 5;

	// 몬스터 무리가 생성되는 간격(초). 모든 스폰 포인트를 같은 값으로 맞추면 매 간격마다 동시에 스폰됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0))
	float SpawnInterval = 5.0f;

	// 웨이브 사이의 대기시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0))
	float WaveInterval = 10.0f;

	// 마지막 웨이브가 끝난 뒤의 "라운드 대기시간"(기획서 기준 15초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0))
	float RoundEndWaitTime = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentSpeed = 100.0f;

	ECPWavePhase CurrentPhase = ECPWavePhase::Spawning;
	int32 CurrentWaveIndex = 0;
	int32 BurstsSpawnedThisWave = 0;

	FTimerHandle SpawnTimer;
	FTimerHandle WaveTimer;
	FTimerHandle RoundEndTimer;
};

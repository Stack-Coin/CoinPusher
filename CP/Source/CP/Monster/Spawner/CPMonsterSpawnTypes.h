// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Monster/Stat/CPMonsterStatTypes.h"
#include "CPMonsterSpawnTypes.generated.h"

UENUM(BlueprintType)
enum class ECPWavePhase : uint8
{
	Spawning     UMETA(DisplayName = "Spawning"),      // 몬스터 무리를 생성하는 중
	WaveWait     UMETA(DisplayName = "Wave Wait"),      // 웨이브 사이 대기 중
	RoundWait    UMETA(DisplayName = "Round Wait"),     // 마지막 웨이브 종료 후 보스 등장 대기 중
	Finished     UMETA(DisplayName = "Finished")        // 라운드(보스 포함) 종료
};

USTRUCT(BlueprintType)
struct FCPSpawnWaveEntryRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 Round = 1;

	/** 라운드 안에서 몇 번째 웨이브인지 (1부터 시작) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 Wave = 1;

	/** 이 규칙을 적용할 스포너 인덱스들 (예: 1,3,5,7,9).
	 *  각 인덱스는 레벨의 ACPMonsterSpawner에 붙인 Tag("Spawner0", "Spawner1"...)의 숫자와 매칭됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<int32> SpawnerIndices;

	/** BaseStatTable/WaveStatTable과 동일한 타입 - 스탯 시스템과 그대로 연결됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	ECPMonsterType MonsterType = ECPMonsterType::Normal;

	/** 스폰 포인트 1곳당 목표 마릿수. 이만큼 스폰되면 이 규칙은 종료됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = 1))
	int32 CountPerSpawnPoint = 1;

	/** 몇 초에 한 번 스폰할지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = 0))
	float SpawnInterval = 1.0f;

	/** 한 번에(같은 타이밍에) 몇 마리씩 스폰할지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = 1))
	int32 MonstersPerSpawn = 1;

	/** MonstersPerSpawn > 1일 때, 스포너를 중심으로 나란히 배치할 월드 Y축 간격(cm) - 원하는 값.
	 *  실제 스폰 시 몬스터의 콜리전 캡슐 지름보다 작으면 자동으로 캡슐 지름만큼 늘어나서 겹쳐서 스폰되지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = 0))
	float SpawnRowSpacingY = 100.0f;

	/** 이 웨이브의 모든 규칙(같은 Round+Wave의 행들)이 끝난 뒤, 다음 웨이브까지 대기시간(초).
	 *  같은 웨이브에 속한 행이라면 전부 같은 값을 넣어주세요 (첫 번째로 발견되는 값을 사용합니다). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = 0))
	float WaveEndWaitTime = 10.0f;
};

/**
 * 라운드 단위 설정. 라운드 1개당 1행입니다.
 * (스포너 링 구성값 + 보스 설정을 함께 담습니다 - 어차피 라운드 전환 시점에 한 번에 적용되는 값들이라
 *  별도 테이블로 쪼개지 않고 여기 모아뒀습니다.)
 */
USTRUCT(BlueprintType)
struct FCPRoundInfoRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	int32 Round = 1;

	/** 이 라운드에서 오너(플레이어)를 중심으로 생성할 스포너 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round", meta = (ClampMin = 0))
	int32 SpawnerCount = 8;

	/** 이 라운드에서 스포너들을 배치할 반경(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round", meta = (ClampMin = 0))
	float SpawnerRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	ECPMonsterType BossMonsterType = ECPMonsterType::Boss;

	/** 기획서: 고정적으로 존재하는 1개의 보스 스폰 포인트 (스포너 인덱스) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	int32 BossSpawnerIndex = 0;

	/** 마지막 웨이브 종료 후, 보스 등장까지 대기시간(초) - 기획서의 "라운드 대기시간" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (ClampMin = 0))
	float RoundEndWaitTime = 10.0f;

	/** 체력이 이 비율 밑으로 떨어지면 포효(무적) 발동 - ACPMonsterBoss::ApplyBossWaveStat으로 전달됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoarHealthPercentThreshold = 0.5f;

	/** 슬램(내려찍기) 공격 쿨타임(초) - ACPMonsterBoss::ApplyBossWaveStat으로 전달됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (ClampMin = 0))
	float SlamCooldown = 4.0f;
};

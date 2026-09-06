// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CPMonsterStatTypes.generated.h"

UENUM(BlueprintType)
enum class ECPMonsterType : uint8
{
	Normal   UMETA(DisplayName = "일반형"),
	Tanker   UMETA(DisplayName = "탱커형"),
	Ranged   UMETA(DisplayName = "원거리형"),
};

USTRUCT(BlueprintType)
struct FCPMonsterDefaultStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float AttackSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float KnockbackDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float DetectRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float CollisionRadius = 0.f;

	/** 정찰 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float PatrolRadius = 0.f;

	/** 공격 사거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float AttackRange = 0.f;

	/** 타겟을 향해 회전하는 속도 (AIController 구현에 맞춰 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float TurnSpeed = 0.f;

	/** MoveTo(BT)로 이동할 때 목표 지점에서 이 반경 안에 들어오면 도착으로 간주함 (cm).
	 * 근접형은 작게, 원거리형은 공격 사거리만큼 크게 잡아서 너무 가까이 붙지 않고 멈추도록 함 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float MoveAcceptableRadius = 0.f;
};

USTRUCT(BlueprintType)
struct FCPMonsterStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 웨이브에 따른 수치 변화 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Wave")
	float MaxHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Wave")
	float MoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Wave")
	float AttackPower = 0.f;

	// 웨이브에 따른 수치 변화 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Deafult")
	FCPMonsterDefaultStat DefaultStat;
};

// 웨이브 1개에 대한 증가치
USTRUCT(BlueprintType)
struct FCPMonsterWaveStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 Wave = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float AddMaxHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float AddMoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float AddAttackPower = 0.f;
};

// RowName은 BaseStatTable과 동일하게 MonsterType(Normal/Tanker/Ranged)을 사용하고,
// 그 안에서 웨이브별 증가치를 배열로 관리
USTRUCT(BlueprintType)
struct FCPMonsterWaveStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FCPMonsterWaveStat> WaveStats;
};

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
	Boss     UMETA(DisplayName = "보스"),
};

USTRUCT(BlueprintType)
struct FCPMonsterDefaultStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float AttackSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float CollisionRadius = 0.f;

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

	/** 다른 몬스터와 겹치지 않게 유지할 최소 여유 간격 (콜리전 반경 합에 추가로 더하는 값, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float SeparationPadding = 70.f;

	/** 겹쳤을 때 밀어내는 최대 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float SeparationSpeed = 400.f;
};

// 엑셀에서 작업하기 편하도록 구조체 수정
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

	// 웨이브에 따른 수치 변화 없음 (기존 FCPMonsterDefaultStat 필드를 그대로 평탄화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float AttackSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float CollisionRadius = 0.f;

	/** 공격 사거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float AttackRange = 0.f;

	/** 타겟을 향해 회전하는 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float TurnSpeed = 0.f;

	/** MoveTo(BT) 목표 지점 도착 판정 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float MoveAcceptableRadius = 0.f;

	/** 다른 몬스터와 유지할 최소 여유 간격 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float SeparationPadding = 70.f;

	/** 겹쳤을 때 밀어내는 최대 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	float SeparationSpeed = 400.f;
};

/**
 * 웨이브 1개에 대한 증가치. RowName은 자유롭게(예: "Normal_W1") 넣고, 실제 조회는
 * MonsterType+Wave 컬럼으로 합니다. 엑셀에서 한 줄 = 특정 타입의 특정 웨이브 증가치입니다.
 */
USTRUCT(BlueprintType)
struct FCPMonsterWaveStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// BaseStatTable과 동일한 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	ECPMonsterType MonsterType = ECPMonsterType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 Wave = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float AddMaxHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float AddMoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float AddAttackPower = 0.f;
};

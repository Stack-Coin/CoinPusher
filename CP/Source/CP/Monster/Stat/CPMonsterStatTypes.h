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

UENUM(BlueprintType)
enum class ECPMonsterMoveType : uint8
{
	Walking   UMETA(DisplayName = "보행"),
	Hovering  UMETA(DisplayName = "부유"),
};

UENUM(BlueprintType)
enum class ECPMonsterAttackType : uint8
{
	Melee      UMETA(DisplayName = "근접 공격"),
	Ranged     UMETA(DisplayName = "원거리"),
};

USTRUCT(BlueprintType)
struct FCPMonsterProjectileStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<AActor> ProjectileClass;
};

/**
 * - 일반형, 탱커형(보행형 + 오버랩)
 * - 원거리형(부유형 + 투사체)
 */
USTRUCT(BlueprintType)
struct FCPMonsterTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	ECPMonsterMoveType MoveType = ECPMonsterMoveType::Walking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	ECPMonsterAttackType AttackType = ECPMonsterAttackType::Melee;

	// AttackType == Projectile일 때만 설정 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset", meta = (EditCondition = "AttackType == ECPMonsterAttackType::Projectile"))
	FCPMonsterProjectileStat ProjectileStat;
};

USTRUCT(BlueprintType)
struct FCPMonsterDefaultStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float AttackSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	float KnockbackPower = 0.f;

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
};

USTRUCT(BlueprintType)
struct FCPMonsterStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 몬스터 템플릿
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	FCPMonsterTemplate Preset;

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

USTRUCT(BlueprintType)
struct FCPMonsterWaveStatRow : public FTableRowBase
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

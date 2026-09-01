// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPStatTypes.generated.h"

/** Identifies a single modifiable player stat */
UENUM(BlueprintType)
enum class ECPStatType : uint8
{
	Health,
	Experience,
	AttackPower,
	MoveSpeed,
	AttackSpeed,
	Defense,
	Level
};

/** Inclusive Min/Max bounds for a single stat. SetStat/ModifyStat clamp to this range, so augments and
 *  debuffs can never push a stat outside the values designers set here. */
USTRUCT(BlueprintType)
struct FCPStatRange
{
	GENERATED_BODY()

	FCPStatRange() = default;
	FCPStatRange(float InMin, float InMax) : Min(InMin), Max(InMax) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Min = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Max = 100.0f;
};

/** Data for all of the player's core stats */
USTRUCT(BlueprintType)
struct FCPPlayerStats
{
	GENERATED_BODY()

	/** Current health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Health = 100.0f;

	/** Current experience accumulated towards the next level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Experience = 0.0f;

	/** Base damage dealt by the basic attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackPower = 10.0f;

	/** Movement speed. Applied to CharacterMovementComponent's MaxWalkSpeed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MoveSpeed = 500.0f;

	/** Attack speed multiplier. Divides the base attack cooldown */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackSpeed = 1.0f;

	/** Defense value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Defense = 0.0f;

	/** Current level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	int32 Level = 1;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CPPlayerStatTableTypes.generated.h"

USTRUCT(BlueprintType)
struct FCPPlayerBaseStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float HealthMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float HealthMax = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackPower = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackPowerMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackPowerMax = 999.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MoveSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MoveSpeedMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MoveSpeedMax = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackSpeedMin = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackSpeedMax = 5.0f;
};

USTRUCT(BlueprintType)
struct FCPPlayerLevelStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float RequiredExperience = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AddHealth = 0.0f;
};

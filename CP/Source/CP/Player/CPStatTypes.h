#pragma once

#include "CoreMinimal.h"
#include "CPStatTypes.generated.h"

//***** �÷��̾� ���� ����
UENUM(BlueprintType)
enum class ECPStatType : uint8
{
	Health,
	Experience,
	AttackPower,
	MoveSpeed,
	AttackSpeed,
	Level
};

//***** ���� ����
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

//***** �÷��̾� ����
USTRUCT(BlueprintType)
struct FCPPlayerStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackPower = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MoveSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Experience = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	int32 Level = 1;
};

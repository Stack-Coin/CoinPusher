// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Monster/Stat/CPMonsterStatTypes.h"
#include "CPMonsterStatComponent.generated.h"

class UDataTable;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CP_API UCPMonsterStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCPMonsterStatComponent();

public:
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void InitStat(ECPMonsterType InMonsterType, int32 InWave);

private:
	static FName GetMonsterTypeRowName(ECPMonsterType InType);
	void ResetStat();

public:
	// 데이터
	UPROPERTY(EditDefaultsOnly, Category = "Stat|DataTable")
	TObjectPtr<UDataTable> BaseStatTable;

	UPROPERTY(EditDefaultsOnly, Category = "Stat|DataTable")
	TObjectPtr<UDataTable> WaveStatTable;

	// 몬스터 타입
	UPROPERTY(EditDefaultsOnly, Category = "Stat|Template")
	FCPMonsterTemplate MonsterTemplete;

	// 읽기 전용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Wave")
	float MaxHealth = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Wave")
	float CurrentHealth = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Wave")
	float MoveSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Wave")
	float AttackPower = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Default")
	FCPMonsterDefaultStat DefaultStat;
};

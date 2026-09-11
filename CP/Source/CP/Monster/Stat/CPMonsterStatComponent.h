// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Monster/Stat/CPMonsterStatTypes.h"
#include "CPMonsterStatComponent.generated.h"

class UDataTable;

/** 체력이 바뀔 때마다(데미지를 받을 때) Broadcast - CPMonsterBase.h의 OnMonsterDied와 같은 패턴.
 *  UCPMonsterSpawnManagerComponent::SpawnBoss()가 보스 스폰 시 구독해서 InGameUI의 보스 체력
 *  게이지를 갱신하는 데 사용함(HandleBossHealthChanged 참고) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMonsterHealthChanged, float, CurrentHealth, float, MaxHealth);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CP_API UCPMonsterStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCPMonsterStatComponent();

public:
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void InitStat(ECPMonsterType InMonsterType, int32 InRound, int32 InWave);

	/** 데미지를 받아 CurrentHealth가 바뀔 때마다 Broadcast (ACPMonsterBase::TakeDamage 참고) */
	UPROPERTY(BlueprintAssignable, Category = "Stat|Events")
	FOnMonsterHealthChanged OnMonsterHealthChanged;

private:
	static FName GetMonsterTypeRowName(ECPMonsterType InType);
	void ResetStat();

public:
	// 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|DataTable")
	TObjectPtr<UDataTable> BaseStatTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|DataTable")
	TObjectPtr<UDataTable> WaveStatTable;

	// 기획자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Wave")
	float MaxHealth = 0.f;

	// 읽기 전용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Wave")
	float CurrentHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Wave")
	float MoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Wave")
	float AttackPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Default")
	FCPMonsterDefaultStat DefaultStat;

	/** true면 InitStat이 DataTable 값으로 덮어쓰지 않고, 디테일 패널에 입력된 값을 그대로 사용함.
	 *  기획자가 특정 인스턴스만 임의의 수치로 테스트해보고 싶을 때 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Override")
	bool bOverrideStat = false;
};

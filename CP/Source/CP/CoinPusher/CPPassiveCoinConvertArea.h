// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinTypes.h"
#include "CPPassiveCoinConvertArea.generated.h"

class UBoxComponent;
class ACPCoin;

/**
 *  레벨에 배치해두는 트리거 볼륨 - 다른 곳(BP, 다른 시스템 등)에서 ConvertActive(Num)을 호출하면
 *  이 영역과 겹쳐 있는 Coin 중 Num개를 랜덤하게 골라 Passive 코인으로 바꾼다.
 *  볼륨 자체는 Overlap만 감지하고(BlockAllDynamic이 아닌 OverlapAllDynamic) 아무것도 물리적으로 막지 않는다.
 */
UCLASS(abstract)
class CP_API ACPPassiveCoinConvertArea : public AActor
{
	GENERATED_BODY()

	//코인과 겹쳤는지만 감지하는 볼륨 (콜리전 없음 - Overlap 전용). 크기는 BP/디테일 패널에서 자유롭게 조정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* ConvertVolume;

public:

	ACPPassiveCoinConvertArea();

	//이 영역과 겹쳐 있는 Coin 중 아직 Passive가 아닌 것들을 대상으로, Num개(가능한 만큼)를 중복 없이
	//랜덤하게 골라 SetCoinType(Passive)를 호출한다
	UFUNCTION(BlueprintCallable, Category="Passive Coin Convert Area")
	void ConvertActive(int32 Num);

	//이 영역과 겹쳐 있는 Normal 코인들을 대상으로, Num개(가능한 만큼)를 중복 없이 랜덤하게 골라
	//SetCoinType(HP)를 호출한다
	UFUNCTION(BlueprintCallable, Category="Passive Coin Convert Area")
	void HPConvertActive(int32 Num);

	//이 영역과 겹쳐 있는 Normal 코인들을 대상으로, Num개(가능한 만큼)를 중복 없이 랜덤하게 골라
	//SetCoinType(Monster)를 호출한다
	UFUNCTION(BlueprintCallable, Category="Passive Coin Convert Area")
	void MonsterConvertActive(int32 Num);

public:

	FORCEINLINE UBoxComponent* GetConvertVolume() const { return ConvertVolume; }

private:

	//ConvertVolume과 겹쳐 있는 Coin 중 조건에 맞는 후보들을 모아 반환
	TArray<ACPCoin*> GatherCandidates(TFunctionRef<bool(const ACPCoin&)> Predicate) const;

	//Candidates 중 Num개(가능한 만큼)를 중복 없이 랜덤하게 골라 TargetType으로 전환
	static void ConvertRandomCandidates(TArray<ACPCoin*> Candidates, int32 Num, ECPCoinType TargetType);
};

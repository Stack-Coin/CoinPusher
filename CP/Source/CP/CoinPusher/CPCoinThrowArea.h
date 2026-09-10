// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinThrowArea.generated.h"

class UBoxComponent;

/**
 *  레벨/CoinPusher가 소유하는 트리거 볼륨 - ActiveThrow()를 호출하면 이 영역과 겹쳐 있는 모든 타입의
 *  Coin에게 월드 기준 X(앞)/Z(위) 방향 속도를 부여해 날려보낸다 (액터 자신의 회전과는 무관).
 *  볼륨 자체는 Overlap만 감지하고(OverlapAllDynamic) 아무것도 물리적으로 막지 않는다.
 */
UCLASS(abstract)
class CP_API ACPCoinThrowArea : public AActor
{
	GENERATED_BODY()

	//코인과 겹쳤는지만 감지하는 볼륨 (콜리전 없음 - Overlap 전용). 크기는 BP/디테일 패널에서 자유롭게 조정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* ThrowVolume;

public:

	ACPCoinThrowArea();

protected:

	//bIsRandomize == false일 때 사용할 고정 위(월드 Z) 방향 힘. bIsRandomize == true일 때는 랜덤 범위의 최대값
	UPROPERTY(EditAnywhere, Category="Coin Throw Area", meta = (ClampMin = 0))
	float MaxUpPower = 800.0f;

	//bIsRandomize == true일 때 위(월드 Z) 방향 힘의 랜덤 범위 최소값. bIsRandomize == false면 사용하지 않음
	UPROPERTY(EditAnywhere, Category="Coin Throw Area", meta = (ClampMin = 0))
	float MinUpPower = 400.0f;

	//bIsRandomize == false일 때 사용할 고정 앞(월드 X) 방향 힘. bIsRandomize == true일 때는 랜덤 범위의 최대값
	UPROPERTY(EditAnywhere, Category="Coin Throw Area", meta = (ClampMin = 0))
	float MaxForwardPower = 600.0f;

	//bIsRandomize == true일 때 앞(월드 X) 방향 힘의 랜덤 범위 최소값. bIsRandomize == false면 사용하지 않음
	UPROPERTY(EditAnywhere, Category="Coin Throw Area", meta = (ClampMin = 0))
	float MinForwardPower = 300.0f;

	//true면 ActiveThrow()마다 Min~Max 사이에서 랜덤한 위/앞 힘을 사용, false면 항상 Max 값을 그대로 사용
	UPROPERTY(EditAnywhere, Category="Coin Throw Area")
	bool bIsRandomize = true;

public:

	//이 영역과 겹쳐 있는 모든 Coin에게 월드 X(앞)/Z(위) 방향 속도를 부여해 날린다
	UFUNCTION(BlueprintCallable, Category="Coin Throw Area")
	void ActiveThrow();

	FORCEINLINE UBoxComponent* GetThrowVolume() const { return ThrowVolume; }
};

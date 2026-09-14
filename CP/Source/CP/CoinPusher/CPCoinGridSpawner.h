// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinGridSpawner.generated.h"

class UBoxComponent;
class ACPCoin;

/**
 *  SpawnVolume(UBoxComponent, RootComponent) 영역 안에 CoinClass를 CoinCount개, Grid + Jitter
 *  방식으로 생성하는 액터. ACPCoinPusher가 UChildActorComponent를 통해 Has-a로 소유한다
 *  (DispenserComponentA/DropZoneComponent 등 다른 CoinPusher 구성 요소와 동일한 패턴).
 *
 *  스스로는 BeginPlay에서 스폰하지 않는다 - ACPCoinPusher::BeginPlay()가 GetCoinGridSpawner()로
 *  이 인스턴스를 찾아 SpawnCoins()를 직접 호출해줘야 실제로 코인이 생긴다 (호출 시점/조건을
 *  CoinPusher 쪽에서 제어하기 위함).
 *
 *  "Grid + Jitter"란: SpawnVolume의 로컬 X/Y 범위를 CoinCount에 맞는 정사각형에 가까운 격자
 *  (Columns x Rows)로 나눠 각 셀 중심에 하나씩 배치하되(Grid), 완전히 균일한 격자로 보이지 않고
 *  코인끼리 정확히 겹쳐 스폰되는 것도 방지하기 위해 각 셀 중심에서 셀 크기의 JitterRatio 비율만큼
 *  X/Y를 무작위로 흔든다(Jitter). bJitterHeight가 true면 Z도 SpawnVolume의 Z 범위 안에서 무작위로
 *  흔들어, 코인들이 완전히 같은 높이에서 겹쳐 스폰되어 물리 시뮬레이션이 튀는 것을 추가로 방지한다.
 *
 *  스폰된 코인에는 발사 속도를 부여하지 않는다(ACPDispenser::SpawnItemClass()의 bLaunch와 달리) -
 *  제자리에 놓인 뒤 SimulatePhysics(ACPCoin 기본값)로 자연스럽게 바닥에 떨어져 자리 잡는다.
 */
UCLASS(abstract)
class CP_API ACPCoinGridSpawner : public AActor
{
	GENERATED_BODY()

	/** Grid + Jitter 스폰 범위를 정의하는 Box 영역 (RootComponent) - "해당 컴포넌트가 소유하는
	 *  Box 컴포넌트"가 바로 이것이며, 이 컴포넌트의 크기(Extent)가 곧 스폰 영역이다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* SpawnVolume;

public:

	ACPCoinGridSpawner();

protected:

	/** 스폰할 코인 클래스 (ACPCoin 상속 BP). 비어있으면 아무것도 스폰하지 않는다 */
	UPROPERTY(EditAnywhere, Category="Coin Grid Spawner")
	TSubclassOf<ACPCoin> CoinClass;

	/** 스폰할 코인 개수(N) */
	UPROPERTY(EditAnywhere, Category="Coin Grid Spawner", meta = (ClampMin = 0))
	int32 CoinCount = 20;

	/** 격자 셀 크기 대비 지터(무작위 흔들림) 비율. 0이면 정확히 격자 위치에, 1이면 셀 절반 크기까지
	 *  X/Y가 무작위로 흔들린다 */
	UPROPERTY(EditAnywhere, Category="Coin Grid Spawner", meta = (ClampMin = 0, ClampMax = 1))
	float JitterRatio = 0.5f;

	/** true면 Z도 SpawnVolume의 Z 범위 안에서 무작위로 흔든다 - 코인들이 완전히 같은 높이에서 겹쳐
	 *  스폰되어 물리가 튀는 것을 방지 */
	UPROPERTY(EditAnywhere, Category="Coin Grid Spawner")
	bool bJitterHeight = true;

public:

	/** SpawnVolume 안에 Grid + Jitter 방식으로 CoinCount개의 CoinClass를 생성한다. CoinClass가
	 *  비어있거나 CoinCount가 0 이하면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Coin Grid Spawner")
	void SpawnCoins();

protected:

	/** Index번째 코인의 스폰 위치를 SpawnVolume 로컬 좌표계 기준으로 계산한다(Grid + Jitter).
	 *  Columns/Rows는 SpawnCoins()가 CoinCount로부터 계산해 넘겨준다 */
	FVector CalculateGridJitterLocalLocation(int32 Index, int32 Columns, int32 Rows) const;

public:

	FORCEINLINE UBoxComponent* GetSpawnVolume() const { return SpawnVolume; }
};

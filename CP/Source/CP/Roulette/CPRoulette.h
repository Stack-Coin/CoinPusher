// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPRouletteTypes.h"
#include "CPRoulette.generated.h"

class USceneComponent;
class UCPRouletteWidget;
class ACPCoinPusher;

/**
 *  원형 돌림판 형태의 룰렛. Roll()이 호출되면 화면에 룰렛 UI(UCPRouletteWidget)를 띄우고,
 *  동일한 확률을 갖는 8개의 칸(Slots) 중 하나를 뽑아 UI가 그 칸에서 멈추도록 연출한 뒤,
 *  결정된 칸에 설정된 아이템을 SpawnCount만큼 SpawnPoint 위치에 스폰한다.
 */
UCLASS(abstract)
class CP_API ACPRoulette : public AActor
{
	GENERATED_BODY()

	/** 스폰될 아이템의 기준 위치/방향, 액터의 RootComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SpawnPoint;

public:

	ACPRoulette();

protected:

	/** 룰렛 칸 개수 (8칸 고정) */
	static constexpr int32 NumSlots = 8;

	/** 8개의 칸. 모두 동일한 확률로 당첨되며, 각각 아이템 ID/클래스/스폰 개수를 가짐 */
	UPROPERTY(EditAnywhere, Category="Roulette", meta = (EditFixedSize))
	TArray<FCPRouletteSlotData> Slots;

	/** Roll() 시 화면에 띄울 룰렛 UI 위젯 클래스 */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TSubclassOf<UCPRouletteWidget> RouletteWidgetClass;

	/** 당첨된 아이템을 실제로 스폰할 CoinPusher. 레벨에서 이 룰렛이 속한 ACPCoinPusher 인스턴스를
	 *  직접 연결해야 하며(InputA/InputB와 동일한 방식의 수동 연결), 당첨 시 이 CoinPusher의
	 *  ItemSpawn(ItemID, SpawnCount)을 호출해 천장 Dispenser 중 하나에서 아이템이 나오게 한다 */
	UPROPERTY(EditInstanceOnly, Category="Roulette")
	TObjectPtr<ACPCoinPusher> CoinPusher;

	/** 로컬 스플릿 스크린의 각 플레이어별로 지연 생성 후 재사용되는 룰렛 UI 위젯 인스턴스 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCPRouletteWidget>> RouletteWidgetInstances;

	/** 스핀이 시작되어 결과가 결정되기 전까지 true. 두 플레이어가 하나의 룰렛을 공유하므로,
	 *  한 플레이어가 돌리는 동안 다른 플레이어가 다시 Roll()을 호출하지 못하도록 막는 잠금 상태 */
	UPROPERTY(Transient)
	bool bIsRolling = false;

public:

	/** 룰렛을 동작시킨다: 8칸 중 하나를 균등 확률로 뽑고, UI로 결과를 보여준 뒤 해당 아이템을 스폰한다.
	 *  이미 스핀 중이면(bIsRolling) 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	void Roll();

	/** 현재 스핀이 진행 중인지 여부 (다른 플레이어의 Roll() 상호작용을 막는 데 사용 가능) */
	UFUNCTION(BlueprintPure, Category="Roulette")
	bool IsRolling() const { return bIsRolling; }

protected:

	/** 로컬 스플릿 스크린의 각 PlayerController마다 룰렛 UI 위젯이 없으면 RouletteWidgetClass로 생성해,
	 *  모든 로컬 플레이어의 위젯 인스턴스를 반환 (동일한 스핀 연출이 모든 화면에 나타나도록 함) */
	TArray<UCPRouletteWidget*> GetOrCreateRouletteWidgets();

	/** 위젯이 스핀을 끝내고 결과를 확정했을 때 호출됨 - 해당 칸의 아이템을 스폰 */
	UFUNCTION()
	void HandleRouletteResultDetermined(int32 ResultIndex);

	/** 지정된 칸의 아이템을 CoinPusher->ItemSpawn(ItemID, SpawnCount)에 위임해 스폰 */
	void SpawnSlotItems(const FCPRouletteSlotData& SlotData);

public:

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }
};

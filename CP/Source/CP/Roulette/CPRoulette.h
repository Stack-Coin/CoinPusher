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
 *  Slots에 설정된 칸 수/확률(Probability)에 따라 하나를 뽑아 UI가 그 칸에서 멈추도록 연출한 뒤,
 *  결정된 칸의 RewardTarget(CoinPusher/GameMode)에 따라 당첨 정보를 전달한다.
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

	/** 룰렛의 칸. 개수와 각 칸의 내용(Probability/ItemID/SpawnCount/RewardTarget)을 에디터에서
	 *  자유롭게 추가/삭제/변경할 수 있다 (칸 개수 고정 아님) */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TArray<FCPRouletteSlotData> Slots;

	/** Roll() 시 화면에 띄울 룰렛 UI 위젯 클래스 */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TSubclassOf<UCPRouletteWidget> RouletteWidgetClass;

	/** RewardTarget이 CoinPusher인 칸이 당첨됐을 때 실제로 스폰을 맡을 CoinPusher. 레벨에서 이
	 *  룰렛이 속한 ACPCoinPusher 인스턴스를 직접 연결해야 하며(InputA/InputB와 동일한 방식의 수동
	 *  연결), 당첨 시 이 CoinPusher의 ItemSpawn(ItemID, SpawnCount)을 호출해 천장 Dispenser 중
	 *  하나에서 아이템이 나오게 한다 */
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

	/** 룰렛을 동작시킨다: 월드의 GameMode가 ACPGameMode라면 티켓을 1개 소모해야 하며(부족하면 실패),
	 *  ACPGameMode가 아닌 경우(테스트 레벨 등)는 티켓 검사 없이 진행한다. Slots의 Probability
	 *  가중치에 따라 칸을 뽑고, UI로 결과를 보여준 뒤 해당 칸의 당첨 정보를 전달한다.
	 *  이미 스핀 중이거나(bIsRolling) 티켓이 부족하면 아무 동작도 하지 않고 false를 반환한다 */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	bool Roll();

	/** 현재 스핀이 진행 중인지 여부 (다른 플레이어의 Roll() 상호작용을 막는 데 사용 가능) */
	UFUNCTION(BlueprintPure, Category="Roulette")
	bool IsRolling() const { return bIsRolling; }

protected:

	/** 로컬 스플릿 스크린의 각 PlayerController마다 룰렛 UI 위젯이 없으면 RouletteWidgetClass로 생성해,
	 *  모든 로컬 플레이어의 위젯 인스턴스를 반환 (동일한 스핀 연출이 모든 화면에 나타나도록 함) */
	TArray<UCPRouletteWidget*> GetOrCreateRouletteWidgets();

	/** Slots의 Probability 가중치에 비례해 당첨 칸 인덱스를 뽑는다. 모든 칸의 Probability 합이
	 *  0 이하면(설정 실수 등) 균등 확률로 대체 */
	int32 PickWeightedSlotIndex() const;

	/** 위젯이 스핀을 끝내고 결과를 확정했을 때 호출됨 - 해당 칸의 당첨 정보를 전달 */
	UFUNCTION()
	void HandleRouletteResultDetermined(int32 ResultIndex);

	/** 당첨된 칸의 정보를 UE_LOG로 표시하고, RewardTarget에 따라 CoinPusher->ItemSpawn() 호출
	 *  또는 GameMode(ICPRouletteRewardReceiver)로 전달 */
	void DeliverSlotReward(const FCPRouletteSlotData& SlotData);

public:

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }
};

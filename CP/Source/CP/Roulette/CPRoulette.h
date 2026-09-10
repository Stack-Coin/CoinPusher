// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPRoulette.generated.h"

class USceneComponent;
class UCPRouletteWidget;
class UDataTable;

/** 룰렛에서 아이템이 뽑힐 때마다 ItemID/개수와 함께 Broadcast */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPRoulettePickedUp, FName, ItemID, int32, Count);

/**
 *  원형 돌림판 형태의 룰렛. Roll()이 호출되면 ItemDataTable에서 bRoulette가 true인 행들을
 *  RouletteProbability 가중치로 추첨해 하나를 뽑고, 화면에 룰렛 UI(UCPRouletteWidget)를 띄워 그
 *  칸에서 멈추도록 연출한 뒤, 뽑힌 ItemID/RouletteSpawnCount를 OnPickedUp으로 Broadcast한다.
 *  Roulette는 그 결과를 누가 어떻게 쓰는지 전혀 모른다 - CoinPusher 등 외부 시스템이 OnPickedUp에
 *  직접 바인딩해서 원하는 대로 처리한다 (예: ACPCoinPusher::LinkedRoulette).
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

	/** 룰렛이 아이템을 뽑을 때 사용할 데이터 테이블 (Row Struct는 FItemData여야 함). bRoulette가
	 *  true인 행만 후보가 되며, RouletteProbability 가중치로 추첨해 RouletteSpawnCount개를 전달한다 */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TObjectPtr<UDataTable> ItemDataTable;

	/** Roll() 시 화면에 띄울 룰렛 UI 위젯 클래스 */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TSubclassOf<UCPRouletteWidget> RouletteWidgetClass;

	/** 로컬 스플릿 스크린의 각 플레이어별로 지연 생성 후 재사용되는 룰렛 UI 위젯 인스턴스 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCPRouletteWidget>> RouletteWidgetInstances;

	/** 스핀이 시작되어 결과가 결정되기 전까지 true. 두 플레이어가 하나의 룰렛을 공유하므로,
	 *  한 플레이어가 돌리는 동안 다른 플레이어가 다시 Roll()을 호출하지 못하도록 막는 잠금 상태 */
	UPROPERTY(Transient)
	bool bIsRolling = false;

	/** Roll() 시점에 추첨으로 확정된 ItemID/개수. 위젯 스핀 애니메이션이 끝나면 이 값을 그대로
	 *  OnPickedUp으로 전달한다 (스핀 도중 ItemDataTable이 바뀌어도 결과가 흔들리지 않도록 캐싱) */
	FName PendingResultItemID;
	int32 PendingResultSpawnCount = 0;

public:

	/** 룰렛을 동작시킨다: 월드의 GameMode가 ACPGameMode라면 팀 티켓을 1개 소모해야 하며(부족하면
	 *  실패), ACPGameMode가 아닌 경우(테스트 레벨 등)는 티켓 검사 없이 진행한다. ItemDataTable에서
	 *  bRoulette가 true인 행을 RouletteProbability 가중치로 추첨한 뒤, UI로 결과를 보여주고
	 *  OnPickedUp을 Broadcast한다. 이미 스핀 중이거나, 티켓이 부족하거나, 뽑을 수 있는 행이 하나도
	 *  없으면 아무 동작도 하지 않고 false를 반환한다 */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	bool Roll();

	/** 현재 스핀이 진행 중인지 여부 (다른 플레이어의 Roll() 상호작용을 막는 데 사용 가능) */
	UFUNCTION(BlueprintPure, Category="Roulette")
	bool IsRolling() const { return bIsRolling; }

	/** 아이템이 뽑힐 때마다 ItemID/개수와 함께 Broadcast. 이 결과를 누가 어떻게 처리할지는 전혀
	 *  모르므로, CoinPusher 등 외부 시스템이 여기에 직접 바인딩해서 사용한다 */
	UPROPERTY(BlueprintAssignable, Category="Roulette")
	FOnCPRoulettePickedUp OnPickedUp;

protected:

	/** 로컬 스플릿 스크린의 각 PlayerController마다 룰렛 UI 위젯이 없으면 RouletteWidgetClass로 생성해,
	 *  모든 로컬 플레이어의 위젯 인스턴스를 반환 (동일한 스핀 연출이 모든 화면에 나타나도록 함) */
	TArray<UCPRouletteWidget*> GetOrCreateRouletteWidgets();

	/** ItemDataTable에서 bRoulette가 true인 행들을 모아 RouletteProbability 가중치로 하나를 추첨한다.
	 *  뽑힌 행의 ItemID/RouletteSpawnCount를 PendingResultItemID/PendingResultSpawnCount에 저장하고,
	 *  그 행이 후보 목록에서 몇 번째였는지(OutResultIndex)와 전체 후보 수(OutCandidateCount)를
	 *  위젯 스핀 연출용으로 반환한다. 후보가 하나도 없으면(ItemDataTable 미지정 포함) false 반환 */
	bool PickWeightedItem(int32& OutResultIndex, int32& OutCandidateCount);

	/** 위젯이 스핀을 끝내고 결과를 확정했을 때 호출됨 - PendingResultItemID/PendingResultSpawnCount를 전달 */
	UFUNCTION()
	void HandleRouletteResultDetermined(int32 ResultIndex);

public:

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }
};

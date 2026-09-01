// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPRouletteTypes.h"
#include "CPRoulette.generated.h"

class USceneComponent;
class UCPRouletteWidget;

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

	/** 스폰 시 앞 방향으로 부여할 속도. 스폰된 액터의 루트가 물리 시뮬레이션 컴포넌트일 때만 적용 */
	UPROPERTY(EditAnywhere, Category="Roulette", meta = (ClampMin = 0, Units = "cm/s"))
	float LaunchForwardSpeed = 0.0f;

	/** 스폰 시 위 방향으로 부여할 속도. 스폰된 액터의 루트가 물리 시뮬레이션 컴포넌트일 때만 적용 */
	UPROPERTY(EditAnywhere, Category="Roulette", meta = (ClampMin = 0, Units = "cm/s"))
	float LaunchUpwardSpeed = 0.0f;

	/** 지연 생성 후 재사용되는 룰렛 UI 위젯 인스턴스 */
	UPROPERTY(Transient)
	TObjectPtr<UCPRouletteWidget> RouletteWidgetInstance;

public:

	/** 룰렛을 동작시킨다: 8칸 중 하나를 균등 확률로 뽑고, UI로 결과를 보여준 뒤 해당 아이템을 스폰한다 */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	void Roll();

protected:

	/** RouletteWidgetInstance가 없으면 RouletteWidgetClass로 생성해 반환. 실패 시 nullptr */
	UCPRouletteWidget* GetOrCreateRouletteWidget();

	/** 위젯이 스핀을 끝내고 결과를 확정했을 때 호출됨 - 해당 칸의 아이템을 스폰 */
	UFUNCTION()
	void HandleRouletteResultDetermined(int32 ResultIndex);

	/** 지정된 칸의 아이템을 SpawnCount만큼 SpawnPoint 위치에 스폰 */
	void SpawnSlotItems(const FCPRouletteSlotData& SlotData);

public:

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }
};

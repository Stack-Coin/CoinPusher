// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPRoulette.generated.h"

class USceneComponent;
class UCPRouletteWidget;
class UDataTable;
class UTexture2D;

/** 룰렛에서 아이템이 뽑힐 때마다 ItemID/개수와 함께 Broadcast */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCPRoulettePickedUp, FName, ItemID, int32, Count);

/**
 *  원형 돌림판 형태의 룰렛. Roll(PlayerLevel)이 호출되면 RouletteDataTable의 각 행을 인덱스로 한
 *  후보 목록 중 하나를 추첨한다 - PlayerLevel과 MustPickLevel이 같은 행이 있으면 그 행(들) 중
 *  균등 확률로 반드시 하나를 당첨시키고, 없으면 RouletteProbabilityDataTable에서 PlayerLevel에
 *  해당하는 행의 Roulette_index0~9 가중치로 후보 인덱스를 추첨한다. 당첨된 행의 ItemID(실제 표시/
 *  스폰 정보는 ItemDataTable에서 조회)/PickEA를 화면에 룰렛 UI(UCPRouletteWidget)를 띄워 그 칸에서
 *  멈추도록 연출한 뒤 OnPickedUp으로 Broadcast한다. Roulette는 그 결과를 누가 어떻게 쓰는지 전혀
 *  모른다 - CoinPusher 등 외부 시스템이 OnPickedUp에 직접 바인딩해서 원하는 대로 처리한다
 *  (예: ACPCoinPusher::LinkedRoulette).
 *
 *  룰렛 UI 위젯은 이 액터가 직접 만들지 않는다 - 로컬 스플릿 스크린의 각 플레이어가 이미 갖고 있는
 *  "InGameUI"(UCPInGameWidget, ACPTopDownPlayerController::GetInGameWidget())의 RouletteWidget
 *  컴포넌트를 그대로 재사용한다(GetLocalRouletteWidgets() 참고) - InGameUI 밖에 별도로 룰렛
 *  위젯을 띄우지 않으므로 WBP 하나만 관리하면 된다.
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

	/** 아이템 마스터 데이터 테이블 (Row Struct는 FItemData여야 함). 당첨된 ItemID의 표시/스폰 정보를
	 *  조회하는 용도로만 쓰이며, 추첨 후보/확률은 RouletteDataTable/RouletteProbabilityDataTable에서 가져온다 */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TObjectPtr<UDataTable> ItemDataTable;

	/** 룰렛 추첨 후보 테이블 (Row Struct는 FCPRouletteDataRow). 각 행이 후보 하나이며, 행의 나열 순서가
	 *  곧 추첨에 쓰이는 인덱스(0..N-1)다 - RouletteProbabilityDataTable의 Roulette_index0~9는 이 인덱스를
	 *  가리킨다. MustPickLevel이 Roll()에 전달된 PlayerLevel과 같은 행이 있으면 그 행(들) 중 균등 확률로
	 *  반드시 당첨시키고, 없으면 RouletteProbabilityDataTable 가중치로 추첨한다 */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TObjectPtr<UDataTable> RouletteDataTable;

	/** 팀 레벨별 룰렛 확률 가중치 테이블 (Row Struct는 FCPRouletteProbabilityRow). 각 행의 Level 필드가
	 *  Roll()에 전달된 PlayerLevel과 일치하는 행을 찾아, 그 행의 Roulette_index0~9 중 RouletteDataTable
	 *  후보 인덱스에 해당하는 값을 가중치로 사용한다. 일치하는 Level 행이 없으면 테이블에 정의된 순서상
	 *  가장 마지막 행의 가중치를 그대로 사용한다 (테이블이 비어있거나 가중치 합이 0 이하면 균등 확률로 대체) */
	UPROPERTY(EditAnywhere, Category="Roulette")
	TObjectPtr<UDataTable> RouletteProbabilityDataTable;

	/** 스핀이 시작되어 결과가 결정되기 전까지 true. 두 플레이어가 하나의 룰렛을 공유하므로,
	 *  한 플레이어가 돌리는 동안 다른 플레이어가 다시 Roll()을 호출하지 못하도록 막는 잠금 상태 */
	UPROPERTY(Transient)
	bool bIsRolling = false;

	/** Roll() 시점에 추첨으로 확정된 ItemID/개수. 위젯 스핀 애니메이션이 끝나면 이 값을 그대로
	 *  OnPickedUp으로 전달한다 (스핀 도중 ItemDataTable이 바뀌어도 결과가 흔들리지 않도록 캐싱) */
	FName PendingResultItemID;
	int32 PendingResultSpawnCount = 0;

	/** Roll() 시점에 추첨으로 확정된 행의 PickUpImage - 스핀이 끝나면 UCPRouletteWidget::PlaySpin에
	 *  그대로 전달되어 PickUp 연출에 사용된다 */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PendingResultPickUpImage;

public:

	/** 룰렛을 동작시킨다: 월드의 GameMode가 ACPGameMode라면 팀 티켓을 1개 소모해야 하며(부족하면
	 *  실패), ACPGameMode가 아닌 경우(테스트 레벨 등)는 티켓 검사 없이 진행한다. PlayerLevel은
	 *  MustPickLevel 강제 당첨 여부와 RouletteProbabilityDataTable의 확률 가중치 조회에 사용된다
	 *  (호출자가 팀 레벨 등을 직접 조회해 넘겨줘야 함). PickWeightedItem(PlayerLevel)으로 추첨한
	 *  뒤, UI로 결과를 보여주고 OnPickedUp을 Broadcast한다. 이미 스핀 중이거나, 티켓이 부족하거나,
	 *  뽑을 수 있는 행이 하나도 없으면 아무 동작도 하지 않고 false를 반환한다 */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	bool Roll(int32 PlayerLevel);

	/** 현재 스핀이 진행 중인지 여부 (다른 플레이어의 Roll() 상호작용을 막는 데 사용 가능) */
	UFUNCTION(BlueprintPure, Category="Roulette")
	bool IsRolling() const { return bIsRolling; }

	/** 아이템이 뽑힐 때마다 ItemID/개수와 함께 Broadcast. 이 결과를 누가 어떻게 처리할지는 전혀
	 *  모르므로, CoinPusher 등 외부 시스템이 여기에 직접 바인딩해서 사용한다 */
	UPROPERTY(BlueprintAssignable, Category="Roulette")
	FOnCPRoulettePickedUp OnPickedUp;

protected:

	/** 로컬 스플릿 스크린의 각 PlayerController(ACPTopDownPlayerController)마다 그 InGameUI
	 *  (GetInGameWidget())의 RouletteWidget을 찾아 모은다 - 위젯을 새로 만들지 않고 이미 존재하는
	 *  인스턴스를 재사용하며, 처음 찾을 때마다 OnResultDetermined를 AddUniqueDynamic으로 바인딩해
	 *  중복 바인딩 없이 항상 최신 상태를 보장한다. InGameUI가 없거나 RouletteWidget이 배치되지
	 *  않은 플레이어는 결과 목록에서 제외된다(그 화면에는 스핀 연출이 나오지 않음) */
	TArray<UCPRouletteWidget*> GetLocalRouletteWidgets();

	/** RouletteDataTable의 각 행을 인덱스(0..N-1) 순서의 후보로 삼아 하나를 추첨한다. 후보 중
	 *  MustPickLevel이 PlayerLevel과 같은 행이 하나 이상 있으면 그 행들 중 균등 확률로 반드시
	 *  하나를 당첨시키고, 없으면 RouletteProbabilityDataTable에서 Level이 PlayerLevel과 같은 행을
	 *  찾아 그 행의 Roulette_index0~9 가중치로 추첨한다 (일치하는 Level 행이 없으면 테이블의 가장
	 *  마지막 행 가중치를 그대로 사용하고, 테이블이 비어있거나 가중치 합이 0 이하면 균등 확률로
	 *  대체). 뽑힌 행의 ItemID/PickEA를 PendingResultItemID/PendingResultSpawnCount에
	 *  저장하고, 후보 목록에서 몇 번째였는지(OutResultIndex)와 전체 후보 수(OutCandidateCount)를
	 *  위젯 스핀 연출용으로 반환한다. 후보가 하나도 없으면(RouletteDataTable 미지정 포함) false 반환 */
	bool PickWeightedItem(int32 PlayerLevel, int32& OutResultIndex, int32& OutCandidateCount);

	/** 위젯이 스핀을 끝내고 결과를 확정했을 때 호출됨 - PendingResultItemID/PendingResultSpawnCount를 전달 */
	UFUNCTION()
	void HandleRouletteResultDetermined(int32 ResultIndex);

public:

	FORCEINLINE USceneComponent* GetSpawnPoint() const { return SpawnPoint; }
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CoinPusher/CPCoinTypes.h"
#include "CPItemData.generated.h"

class AActor;

/** 아이템 마스터 데이터 한 행. UCPItemDataTableGameInstance::ItemDataTable의 Row Struct로 쓰인다 */
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	/** 식별용 아이템 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FName ID;

	/** 아이템 대분류 (예: Coin, Item, Equipment 등 - 프로젝트에서 자유롭게 정의) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FName Category;

	/** Category 안에서의 세부 종류 (예: Category가 Coin이면 Normal/Big/Monster 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FName Type;

	/** 화면에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	FText Name;

	/** 이 아이템이 코인일 때 적용할 코인 타입 (코인이 아니면 무시됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	ECPCoinType CoinType = ECPCoinType::Normal;

	/** ACPCoinPusher(천장 Dispenser 등)에서 이 아이템을 스폰할 때 사용할 액터 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	TSubclassOf<AActor> CoinPusherSpawnBPClass;

	/** 월드에 스폰할 때 사용할 액터 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TSubclassOf<AActor> WorldSpawnBPClass;

	/** 이 아이템이 룰렛에서 당첨될 수 있는 후보인지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	bool bRoulette = false;

	/** 룰렛 당첨 확률 가중치. bRoulette가 true인 모든 행의 RouletteProbability 합(TotalProbability)
	 *  대비 이 행의 비율로 당첨 확률이 결정되며, 합이 반드시 1일 필요는 없다. bRoulette가 false면 무시됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	float RouletteProbability = 0.0f;

	/** 룰렛에서 이 아이템이 당첨됐을 때 스폰(또는 전달)할 개수. bRoulette가 false면 무시됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Data")
	int32 RouletteSpawnCount = 1;
};

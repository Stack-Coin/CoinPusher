// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CPRouletteDataTypes.generated.h"

/**
 *  룰렛 항목별 부가 규칙 한 행. ACPRoulette가 실제 당첨 후보를 추첨하는 데 쓰는 ItemDataTable
 *  (Datatables/CPItemData.h의 FItemData)과는 완전히 별개의 신규 테이블로, "당첨 시 몇 개를
 *  줄지", "CoinPusher로 전달되는지", "몇 레벨부터 뽑힐 수 있는지"처럼 룰렛에서만 의미 있는 부가
 *  규칙을 ItemID로 매칭해서 관리한다 (ItemDataTable/FItemData는 건드리지 않는다).
 */
USTRUCT(BlueprintType)
struct FCPRouletteDataRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 이 행이 어떤 항목에 대한 규칙인지 식별하는 ID (ItemDataTable의 ID와 매칭해서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	FName ItemID;

	/** 룰렛에서 이 ItemID가 당첨됐을 때 지급할 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	int32 PickEA = 1;

	/** 룰렛에서 이 ItemID가 당첨됐을 때 CoinPusher로 전달되는지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	bool bRouletteToCoinPusher = false;

	/** 이 ItemID가 당첨 후보로 뽑히기 위해 팀이 최소로 도달해 있어야 하는 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	int32 MustPickLevel = 1;
};

/**
 *  항목(Row Name = RouletteDataTable의 ItemID)별로 팀 레벨 1~9 각각에 적용할 룰렛 확률
 *  가중치를 한 행에 담아 관리한다. 레벨별로 행을 나누는 대신 레벨 수만큼 컬럼을 두는 구조.
 *  이 값을 실제 추첨에 어떻게 반영할지는 아직 ACPRoulette 쪽에 구현돼 있지 않음 - 데이터
 *  스키마만 우선 정의해둔 상태이며, 추첨 로직 반영은 별도 작업이 필요하다.
 */
USTRUCT(BlueprintType)
struct FCPRouletteProbabilityRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 팀 레벨 1에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level1 = 0.0f;

	/** 팀 레벨 2에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level2 = 0.0f;

	/** 팀 레벨 3에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level3 = 0.0f;

	/** 팀 레벨 4에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level4 = 0.0f;

	/** 팀 레벨 5에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level5 = 0.0f;

	/** 팀 레벨 6에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level6 = 0.0f;

	/** 팀 레벨 7에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level7 = 0.0f;

	/** 팀 레벨 8에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level8 = 0.0f;

	/** 팀 레벨 9에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RouletteProbability_Level9 = 0.0f;
};

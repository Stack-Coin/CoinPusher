// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CPRouletteDataTypes.generated.h"

/**
 *  룰렛 항목별 부가 규칙 한 행. ACPRoulette가 실제 당첨 후보를 추첨하는 데 쓰는 ItemDataTable
 *  (Datatables/CPItemData.h의 FItemData)과는 완전히 별개의 신규 테이블로, "이 항목이 당첨되면
 *  CoinPusher로 전달되는지"와 "몇 레벨부터 뽑힐 수 있는지"처럼 룰렛에서만 의미 있는 부가 규칙을
 *  ID로 매칭해서 관리한다 (ItemDataTable/FItemData는 건드리지 않는다).
 */
USTRUCT(BlueprintType)
struct FCPRouletteDataRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 이 행이 어떤 항목에 대한 규칙인지 식별하는 ID (ItemDataTable의 ID와 매칭해서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	FName ID;

	/** 룰렛에서 이 ID가 당첨됐을 때 CoinPusher로 전달되는지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	bool bRouletteToCoinPusher = false;

	/** 이 ID가 당첨 후보로 뽑히기 위해 팀이 최소로 도달해 있어야 하는 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	int32 MustLevelPick = 1;
};

/**
 *  팀 레벨별로 룰렛에 적용할 확률 가중치 한 행. 레벨 1~9 각각 1행씩, 총 9행으로 채워 쓴다.
 *  이 값을 실제 추첨에 어떻게 반영할지(예: MustLevelPick을 만족하는 후보들의 가중치에 팀의 현재
 *  Level에 해당하는 Probability를 추가로 곱하는 등)는 아직 ACPRoulette 쪽에 구현돼 있지 않음 -
 *  데이터 스키마만 우선 정의해둔 상태이며, 추첨 로직 반영은 별도 작업이 필요하다.
 */
USTRUCT(BlueprintType)
struct FCPLevelRouletteProbabilityRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 팀 레벨 (1~9) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 1))
	int32 Level = 1;

	/** 이 레벨에 적용할 확률 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Probability = 0.0f;
};

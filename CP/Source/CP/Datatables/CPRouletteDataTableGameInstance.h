// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Datatables/CPItemDataTableGameInstance.h"
#include "CPRouletteDataTableGameInstance.generated.h"

class UDataTable;

/**
 *  룰렛에서만 의미 있는 추가 데이터 테이블 2개(RouletteDataTable/LevelRouletteProbabilityDataTable,
 *  Row Struct는 Roulette/CPRouletteDataTypes.h의 FCPRouletteDataRow/FCPLevelRouletteProbabilityRow)를
 *  들고 있는 전용 GameInstance 클래스. 기존 UCPItemDataTableGameInstance(ItemDataTable)는 전혀
 *  건드리지 않고 그대로 상속만 해서 확장한다 - ItemDataTable은 이 클래스에서도 그대로
 *  GetItemDataTable()로 조회 가능.
 *
 *  실제 프로젝트에 적용하려면 기존 BP_ItemDataTableGameInstance(또는 그 상속 BP)의 부모 클래스를
 *  이 클래스로 재부모(Reparent)하면 된다 - Project Settings의 Game Instance Class 자체는 그대로
 *  둔 채 BP만 재부모하면 되므로 별도 설정 변경이 필요 없다.
 */
UCLASS(abstract)
class CP_API UCPRouletteDataTableGameInstance : public UCPItemDataTableGameInstance
{
	GENERATED_BODY()

protected:

	/** 룰렛 항목별 부가 규칙 테이블 (Row Struct는 FCPRouletteDataRow) */
	UPROPERTY(EditDefaultsOnly, Category="Roulette Data")
	TObjectPtr<UDataTable> RouletteDataTable;

	/** 팀 레벨(1~9)별 룰렛 확률 가중치 테이블 (Row Struct는 FCPLevelRouletteProbabilityRow) */
	UPROPERTY(EditDefaultsOnly, Category="Roulette Data")
	TObjectPtr<UDataTable> LevelRouletteProbabilityDataTable;

public:

	FORCEINLINE UDataTable* GetRouletteDataTable() const { return RouletteDataTable; }

	FORCEINLINE UDataTable* GetLevelRouletteProbabilityDataTable() const { return LevelRouletteProbabilityDataTable; }
};

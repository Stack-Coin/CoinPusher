// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CPRouletteTypes.generated.h"

class AActor;

/** 룰렛 한 칸의 데이터. ItemClass가 당첨되면 SpawnCount만큼 스폰된다 */
USTRUCT(BlueprintType)
struct FCPRouletteSlotData
{
	GENERATED_BODY()

	/** 식별용 아이템 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	FName ItemID;

	/** 당첨 시 스폰할 액터 클래스. ICPCoinPusherItem을 구현해야 함 (ACPCoin, ACPItem 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	TSubclassOf<AActor> ItemClass;

	/** 스폰 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0))
	int32 SpawnCount = 1;
};

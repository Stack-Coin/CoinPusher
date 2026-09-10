// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CPRouletteTypes.generated.h"

/** 룰렛 한 칸의 당첨 정보를 누가 받아 처리할지 */
UENUM(BlueprintType)
enum class ECPRouletteRewardTarget : uint8
{
	/** CoinPusher->ItemSpawn(ItemID, SpawnCount)를 호출해 천장 Dispenser에서 실제로 스폰 */
	CoinPusher,
	/** 실제로 스폰하지 않고, GameMode(ICPRouletteRewardReceiver)에게 ItemID/SpawnCount만 전달 */
	GameMode
};

/** 룰렛 한 칸의 데이터. Probability 가중치에 비례해 당첨되며, 당첨되면 RewardTarget에 따라
 *  CoinPusher 스폰 또는 GameMode 전달로 ItemID/SpawnCount가 처리된다 */
USTRUCT(BlueprintType)
struct FCPRouletteSlotData
{
	GENERATED_BODY()

	/** 당첨 확률 가중치. 모든 칸의 Probability 합 대비 이 칸의 비율로 당첨 확률이 결정되며,
	 *  합이 반드시 1일 필요는 없다 (예: 10/20/30/40이면 각각 10%/20%/30%/40%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0))
	float Probability = 1.0f;

	/** 식별용 아이템 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	FName ItemID;

	/** 스폰(또는 전달)할 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette", meta = (ClampMin = 0))
	int32 SpawnCount = 1;

	/** 당첨 정보를 받을 곳 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roulette")
	ECPRouletteRewardTarget RewardTarget = ECPRouletteRewardTarget::CoinPusher;
};

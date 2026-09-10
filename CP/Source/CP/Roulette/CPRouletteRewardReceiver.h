// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoinPusher/CPCoinTypes.h"
#include "CPRouletteRewardReceiver.generated.h"

/**
 *  CPRouletteRewardReceiver
 *  ACPRoulette 칸의 RewardTarget이 GameMode일 때 당첨 정보를 전달받는 인터페이스.
 *  ACPRoulette은 GetAuthGameMode()가 이 인터페이스를 구현하는지만 확인하므로, 실제 GameMode(ACPGameMode)든
 *  테스트용 GameMode든 이 인터페이스만 구현하면 룰렛 당첨 정보를 받을 수 있다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPRouletteRewardReceiver : public UInterface
{
	GENERATED_BODY()
};

class ICPRouletteRewardReceiver
{
	GENERATED_BODY()

public:

	/** 룰렛에서 RewardTarget이 GameMode인 칸이 당첨되었을 때 호출됨. ItemID가 코인이면 CoinType도
	 *  함께 전달되고(코인이 아니면 기본값 Normal) */
	UFUNCTION(BlueprintCallable, Category="Roulette")
	virtual void ReceiveRouletteReward(FName ItemID, int32 SpawnCount, ECPCoinType CoinType = ECPCoinType::Normal) = 0;
};

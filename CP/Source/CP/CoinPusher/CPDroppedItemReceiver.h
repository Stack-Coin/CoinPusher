// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPCoinTypes.h"
#include "CPDroppedItemReceiver.generated.h"

/**
 *  CPDroppedItemReceiver
 *  ACPDropZone에 아이템(코인 포함)이 떨어졌을 때 그 정보를 전달받는 인터페이스. ACPDropZone은
 *  GetAuthGameMode()가 이 인터페이스를 구현하는지만 확인하므로, 실제 게임의 GameMode든 테스트용
 *  GameMode든 이 인터페이스만 구현하면 드랍 정보를 받을 수 있다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPDroppedItemReceiver : public UInterface
{
	GENERATED_BODY()
};

class ICPDroppedItemReceiver
{
	GENERATED_BODY()

public:

	/** DropZone에 ItemID가 Count개 떨어졌을 때 호출됨. ItemID가 코인이면(코인 ItemID와 일치하면)
	 *  CoinType도 함께 전달되고, 코인이 아니면 CoinType은 기본값 Normal로 넘어온다 */
	UFUNCTION(BlueprintCallable, Category="CoinPusher")
	virtual void ReceiveDroppedItem(FName ItemID, int32 Count, ECPCoinType CoinType = ECPCoinType::Normal) = 0;
};

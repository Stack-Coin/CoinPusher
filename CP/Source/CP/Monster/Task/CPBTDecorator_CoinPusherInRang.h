// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CPBTDecorator_CoinPusherInRang.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPBTDecorator_CoinPusherInRang : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UCPBTDecorator_CoinPusherInRang();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

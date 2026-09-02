// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CPBTDecorator_NexusAttackInRange.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPBTDecorator_NexusAttackInRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UCPBTDecorator_NexusAttackInRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

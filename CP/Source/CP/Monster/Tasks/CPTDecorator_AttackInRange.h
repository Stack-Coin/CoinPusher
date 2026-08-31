// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CPTDecorator_AttackInRange.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPTDecorator_AttackInRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UCPTDecorator_AttackInRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

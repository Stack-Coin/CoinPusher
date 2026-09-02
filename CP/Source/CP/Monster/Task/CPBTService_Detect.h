// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "CPBTService_Detect.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPBTService_Detect : public UBTService
{
	GENERATED_BODY()
	
public:
	UCPBTService_Detect();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

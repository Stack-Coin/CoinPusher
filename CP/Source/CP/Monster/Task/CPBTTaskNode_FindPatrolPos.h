// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CPBTTaskNode_FindPatrolPos.generated.h"

/**
 * 
 */
UCLASS()
class CP_API UCPBTTaskNode_FindPatrolPos : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UCPBTTaskNode_FindPatrolPos();

public:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

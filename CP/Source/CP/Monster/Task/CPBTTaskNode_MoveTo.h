// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "CPBTTaskNode_MoveTo.generated.h"

UCLASS()
class CP_API UCPBTTaskNode_MoveTo : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UCPBTTaskNode_MoveTo();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

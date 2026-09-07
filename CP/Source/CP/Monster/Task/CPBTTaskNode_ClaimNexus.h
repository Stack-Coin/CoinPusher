// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CPBTTaskNode_ClaimNexus.generated.h"

UCLASS()
class CP_API UCPBTTaskNode_ClaimNexus : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCPBTTaskNode_ClaimNexus();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

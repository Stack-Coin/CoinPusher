// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "CPBTService_FindCloseNexus.generated.h"

UCLASS()
class CP_API UCPBTService_FindCloseNexus : public UBTService
{
	GENERATED_BODY()

public:
	UCPBTService_FindCloseNexus();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

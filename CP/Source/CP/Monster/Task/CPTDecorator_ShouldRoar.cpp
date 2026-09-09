// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPTDecorator_ShouldRoar.h"
#include "AIController.h"
#include "Monster/Boss/CPMonsterBoss.h"

UCPTDecorator_ShouldRoar::UCPTDecorator_ShouldRoar()
{
	NodeName = TEXT("ShouldRoar");
}

bool UCPTDecorator_ShouldRoar::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return false;
	}

	ACPMonsterBoss* Boss = Cast<ACPMonsterBoss>(AIController->GetPawn());
	if (Boss == nullptr)
	{
		return false;
	}

	return Boss->ShouldRoar();
}

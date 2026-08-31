// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Tasks/CPBTTaskNode_FindPatrolPos.h"
#include "CPAI.h"
#include "../CPMonsterAIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UCPBTTaskNode_FindPatrolPos::UCPBTTaskNode_FindPatrolPos()
{
}

EBTNodeResult::Type UCPBTTaskNode_FindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(ControllingPawn->GetWorld());
	if (NavSystem == nullptr) 
	{
		return EBTNodeResult::Failed;
	}

	FVector Origin = OwnerComp.GetBlackboardComponent()->GetValueAsVector(BBKEY_SPAWNPOS);
	FNavLocation NextPatrolPos;
	
	if (NavSystem->GetRandomPointInNavigableRadius(Origin, 500.0f, NextPatrolPos)) 
	{
		OwnerComp.GetBlackboardComponent()->SetValueAsVector(BBKEY_PATROLPOS, NextPatrolPos.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

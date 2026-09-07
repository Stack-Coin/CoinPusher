// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Task/CPBTTaskNode_MoveTo.h"
#include "AIController.h"
#include "Monster/CPMonsterAIInterface.h"

UCPBTTaskNode_MoveTo::UCPBTTaskNode_MoveTo()
{
	NodeName = TEXT("AcceptableRadius 기반 MoveTo");
}

EBTNodeResult::Type UCPBTTaskNode_MoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(AIController->GetPawn()))
		{
			const float StatAcceptableRadius = AIPawn->GetAIMoveAcceptableRadius();
			if (StatAcceptableRadius > 0.f)
			{
				AcceptableRadius = StatAcceptableRadius;
			}
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

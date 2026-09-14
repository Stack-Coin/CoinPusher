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
			if (StatAcceptableRadius >= 0.f)
			{
				AcceptableRadius = StatAcceptableRadius;
			}

			AIPawn->SetAIState(ECPMonsterAIState::Move);
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

EBTNodeResult::Type UCPBTTaskNode_MoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const EBTNodeResult::Type Result = Super::AbortTask(OwnerComp, NodeMemory);

	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(AIController->GetPawn()))
		{
			AIPawn->SetAIState(ECPMonsterAIState::Idle);
		}
	}

	return Result;
}

void UCPBTTaskNode_MoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(AIController->GetPawn()))
		{
			AIPawn->SetAIState(ECPMonsterAIState::Idle);
		}
	}
}

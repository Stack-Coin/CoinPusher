// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTTaskNode_Roar.h"
#include "AIController.h"
#include "Monster/Boss/CPMonsterBoss.h"

UCPBTTaskNode_Roar::UCPBTTaskNode_Roar()
{
	NodeName = TEXT("Roar");
}

EBTNodeResult::Type UCPBTTaskNode_Roar::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ACPMonsterBoss* Boss = AIController ? Cast<ACPMonsterBoss>(AIController->GetPawn()) : nullptr;
	if (Boss == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	FAICharacterAttackFinished OnRoarFinished;
	OnRoarFinished.BindLambda(
		[this, &OwnerComp]()
		{
			if (AAIController* FinishedController = OwnerComp.GetAIOwner())
			{
				if (ACPMonsterBoss* FinishedBoss = Cast<ACPMonsterBoss>(FinishedController->GetPawn()))
				{
					FinishedBoss->SetAIState(ECPMonsterAIState::Idle);
				}
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	);

	Boss->SetRoarDelegate(OnRoarFinished);
	Boss->SetAIState(ECPMonsterAIState::Roar);
	Boss->RoarByAI();

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCPBTTaskNode_Roar::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (ACPMonsterBoss* Boss = AIController ? Cast<ACPMonsterBoss>(AIController->GetPawn()) : nullptr)
	{
		Boss->CancelRoar();
		Boss->SetAIState(ECPMonsterAIState::Idle);
	}

	return EBTNodeResult::Aborted;
}

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
		[&]()
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	);

	Boss->SetRoarDelegate(OnRoarFinished);
	Boss->RoarByAI();

	return EBTNodeResult::InProgress;
}

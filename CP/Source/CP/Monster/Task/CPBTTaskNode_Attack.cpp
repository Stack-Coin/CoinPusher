// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTTaskNode_Attack.h"
#include "AIController.h"
#include "../CPMonsterAIInterface.h"

UCPBTTaskNode_Attack::UCPBTTaskNode_Attack()
{
}

EBTNodeResult::Type UCPBTTaskNode_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	APawn* ControllingPawn = Cast<APawn>(OwnerComp.GetAIOwner()->GetPawn());
	if (nullptr == ControllingPawn)
	{
		return EBTNodeResult::Failed;
	}

	ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(ControllingPawn);
	if (nullptr == AIPawn)
	{
		return EBTNodeResult::Failed;
	}

	FAICharacterAttackFinished OnAttackFinished;
	OnAttackFinished.BindLambda(
		[&]()
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	);

	// todo. 타이밍 기반으로 Notify로 전달할지.
	/*AIPawn->SetAIAttackDelegate(OnAttackFinished);
	AIPawn->AttackByAI();*/

	return EBTNodeResult::InProgress;
}

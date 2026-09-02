// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPTDecorator_AttackInRange.h"
#include "CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../CPMonsterAIInterface.h"
#include "../../Player/CPPlayerCharacter.h"

UCPTDecorator_AttackInRange::UCPTDecorator_AttackInRange()
{
	NodeName = TEXT("CanAttackPlayer");
}

bool UCPTDecorator_AttackInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	bool bResult = Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return false;
	}

	ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(ControllingPawn);
	if (AIPawn == nullptr)
	{
		return false;
	}

	ACPPlayerCharacter* Target = Cast<ACPPlayerCharacter>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BBKEY_TARGET));
	if (Target == nullptr)
	{
		return false;
	}

	float DistanceToTarget = ControllingPawn->GetDistanceTo(Target);
	float AttackRangeWithRadius = AIPawn->GetAIAttackRange();

	bResult = (DistanceToTarget <= AttackRangeWithRadius);
	return bResult;
}

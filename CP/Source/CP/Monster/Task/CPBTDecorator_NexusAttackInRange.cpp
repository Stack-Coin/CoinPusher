// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTDecorator_NexusAttackInRange.h"
#include "CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../CPMonsterAIInterface.h"

UCPBTDecorator_NexusAttackInRange::UCPBTDecorator_NexusAttackInRange()
{
	NodeName = TEXT("CanAttackNexus");
}

bool UCPBTDecorator_NexusAttackInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
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

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BBKEY_NEXUS));
	if (Target == nullptr)
	{
		return false;
	}

	float DistanceToTarget = ControllingPawn->GetDistanceTo(Target);
	float AttackRangeWithRadius = 300.0f;

	bResult = (DistanceToTarget <= AttackRangeWithRadius);
	return bResult;
}

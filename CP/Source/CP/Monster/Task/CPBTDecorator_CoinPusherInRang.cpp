// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTDecorator_CoinPusherInRang.h"
#include "CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../CPMonsterAIInterface.h"

UCPBTDecorator_CoinPusherInRang::UCPBTDecorator_CoinPusherInRang()
{
	NodeName = TEXT("CanAttackCoinPusher");
}

bool UCPBTDecorator_CoinPusherInRang::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
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

	const FVector PatrolPos = OwnerComp.GetBlackboardComponent()->GetValueAsVector(BBKEY_PATROLPOS);
	
	const float DistanceToTarget = FVector::Dist2D(ControllingPawn->GetActorLocation(), PatrolPos);
	float AttackRangeWithRadius = 7000.f;

	bResult = (DistanceToTarget <= AttackRangeWithRadius);
	return bResult;
}

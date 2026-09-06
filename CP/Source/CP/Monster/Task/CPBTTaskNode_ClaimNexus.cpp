// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Task/CPBTTaskNode_ClaimNexus.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "CPAI.h"
#include "Nexus/CPNexus.h"

UCPBTTaskNode_ClaimNexus::UCPBTTaskNode_ClaimNexus()
{
	NodeName = TEXT("ClaimNexus");
}

EBTNodeResult::Type UCPBTTaskNode_ClaimNexus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!ControllingPawn)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	ACPNexus* ClosestNexus = ACPNexus::FindClosestLivingNexus(this, ControllingPawn->GetActorLocation());
	if (!ClosestNexus)
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsObject(BBKEY_NEXUS, ClosestNexus);

	FVector PatrolPos = ClosestNexus->GetActorLocation();
	PatrolPos.Z = 0.f;
	Blackboard->SetValueAsVector(BBKEY_PATROLPOS, PatrolPos);

	return EBTNodeResult::Succeeded;
}

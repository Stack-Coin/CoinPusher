// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Task/CPBTService_FindCloseNexus.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CPAI.h"
#include "Nexus/CPNexus.h"

UCPBTService_FindCloseNexus::UCPBTService_FindCloseNexus()
{
	NodeName = TEXT("FindCloseNexus (Service)");
	Interval = 0.5f;
}

void UCPBTService_FindCloseNexus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* ControllingPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!ControllingPawn)
	{
		return;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	// Nexus 없음
	ACPNexus* ClosestNexus = ACPNexus::FindClosestLivingNexus(this, ControllingPawn->GetActorLocation());
	if (!ClosestNexus)
	{
		return;
	}

	// 이미 해당 Nexus를 찾고 있음
	AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BBKEY_NEXUS));
	if (ClosestNexus == CurrentTarget)
	{
		return;
	}

	// 웨이포인트로 배회하던 중이었어도, Nexus가 발견되면 즉시 타겟을 갱신함
	Blackboard->SetValueAsObject(BBKEY_NEXUS, ClosestNexus);

	FVector PatrolPos = ClosestNexus->GetActorLocation();
	PatrolPos.Z = 0.f;
	Blackboard->SetValueAsVector(BBKEY_PATROLPOS, PatrolPos);
}

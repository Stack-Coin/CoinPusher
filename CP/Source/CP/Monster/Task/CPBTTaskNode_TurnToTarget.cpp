// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTTaskNode_TurnToTarget.h"
#include "CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../CPMonsterAIInterface.h"

UCPBTTaskNode_TurnToTarget::UCPBTTaskNode_TurnToTarget()
{
	NodeName = TEXT("Turn");
}

EBTNodeResult::Type UCPBTTaskNode_TurnToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	APawn* ControllingPawn = Cast<APawn>(OwnerComp.GetAIOwner()->GetPawn());
	if (ControllingPawn == nullptr)  
	{
		return EBTNodeResult::Failed;
	}

	ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(ControllingPawn);
	if (AIPawn == nullptr) 
	{
		return EBTNodeResult::Failed;
	}

	APawn* TargetPawn = Cast<APawn>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BBKEY_TARGET));
	if (TargetPawn == nullptr) 
	{
		return EBTNodeResult::Failed;
	}
	
	FVector LookVector = TargetPawn->GetActorLocation() - ControllingPawn->GetActorLocation();
	LookVector.Z = 0.f;

	float TurnSpeed = AIPawn->GetAITurnSpeed();

	FRotator TargetRot = FRotationMatrix::MakeFromX(LookVector).Rotator();
	ControllingPawn->SetActorRotation(FMath::RInterpTo(ControllingPawn->GetActorRotation(), TargetRot, GetWorld()->GetDeltaSeconds(), TurnSpeed));

	return EBTNodeResult::Succeeded;
}

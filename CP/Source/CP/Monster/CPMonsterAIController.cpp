// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/CPMonsterAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"

ACPMonsterAIController::ACPMonsterAIController()
{
	// Data
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTAssetRef(TEXT("/Script/AIModule.BehaviorTree'/Game/Monster/AI/BT_MonsterBase.BT_MonsterBase'"));
	if (BTAssetRef.Object != nullptr)
	{
		MonsterBT = BTAssetRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBlackboardData> BBAssetRef(TEXT("/Script/AIModule.BlackboardData'/Game/Monster/AI/BB_MonsterBase.BB_MonsterBase'"));
	if (BBAssetRef.Object != nullptr) 
	{
		MonsterBB = BBAssetRef.Object;
	}
}

void ACPMonsterAIController::RunAI()
{
	UBlackboardComponent* BBComp = Blackboard.Get();
	if (UseBlackboard(MonsterBB, BBComp)) 
	{
		bool bResult = RunBehaviorTree(MonsterBT);
		ensure(bResult);
	}
}

void ACPMonsterAIController::StopAI()
{
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComp)
	{
		BTComp->StopTree();
	}
}

void ACPMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RunAI();
}

void ACPMonsterAIController::OnUnPossess()
{
	StopAI();

	Super::OnUnPossess();
}

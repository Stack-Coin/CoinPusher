// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Task/CPBTTaskNode_MoveTo.h"
#include "AIController.h"
#include "Monster/CPMonsterAIInterface.h"
#include "Monster/CPMonsterAIController.h"

UCPBTTaskNode_MoveTo::UCPBTTaskNode_MoveTo()
{
	NodeName = TEXT("AcceptableRadius 기반 MoveTo");
}

EBTNodeResult::Type UCPBTTaskNode_MoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(AIController->GetPawn()))
		{
			const float StatAcceptableRadius = AIPawn->GetAIMoveAcceptableRadius();
			if (StatAcceptableRadius >= 0.f)
			{
				AcceptableRadius = StatAcceptableRadius;
			}

			AIPawn->SetAIState(ECPMonsterAIState::Move);
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

EBTNodeResult::Type UCPBTTaskNode_MoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const EBTNodeResult::Type Result = Super::AbortTask(OwnerComp, NodeMemory);

	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(AIController->GetPawn()))
		{
			AIPawn->SetAIState(ECPMonsterAIState::Idle);
		}
	}

	return Result;
}

void UCPBTTaskNode_MoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(AIController->GetPawn()))
		{
			AIPawn->SetAIState(ECPMonsterAIState::Idle);
		}

		UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s MoveTo OnTaskFinished Result=%d"), *GetNameSafe(AIController->GetPawn()), static_cast<int32>(TaskResult));

		// MoveTo 실패(On Fail) 시 NavMesh 이탈이 원인일 수 있으므로 다음 Tick까지 기다리지 않고 즉시 복구 시도
		if (TaskResult == EBTNodeResult::Failed)
		{
			if (ACPMonsterAIController* MonsterAIController = Cast<ACPMonsterAIController>(AIController))
			{
				const bool bRecovered = MonsterAIController->TryRecoverFromOffNavMesh();
				UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s OnFail recovery=%s"), *GetNameSafe(AIController->GetPawn()), bRecovered ? TEXT("true") : TEXT("false"));
			}
		}
	}
}

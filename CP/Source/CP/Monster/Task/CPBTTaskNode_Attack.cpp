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
	if (ControllingPawn == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(ControllingPawn);
	if (AIPawn == nullptr)
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

	AIPawn->SetAIAttackDelegate(OnAttackFinished);
	AIPawn->AttackByAI();

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCPBTTaskNode_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 타겟이 사라지는 등의 이유로 상위 데코레이터가 이 태스크를 강제 중단시키는 경우.
	// 몽타주가 자연 종료(NotifyAttackActionEnd)될 기회를 못 얻으므로, 여기서 직접 정리해줘야
	// Attacking CC 상태가 풀리지 않고 애니메이션이 공격 포즈에 멈춰있는 문제를 막을 수 있음
	APawn* ControllingPawn = Cast<APawn>(OwnerComp.GetAIOwner()->GetPawn());
	if (ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(ControllingPawn))
	{
		AIPawn->CancelAIAttack();
	}

	return EBTNodeResult::Aborted;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPTDecorator_AttackInRange.h"
#include "CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
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

	// GetDistanceTo는 두 액터의 피벗(캡슐 중심) 사이 거리라서, DataTable의 AttackRange를 몬스터마다
	// "피벗 기준 절대 거리"로 일일이 맞춰 넣어야 했음(보스처럼 캡슐이 크면 항상 크게 잡아야 함).
	// 대신 여기서 자신과 타겟의 콜리전 반경을 더해줘서, AttackRange는 "몸통 표면끼리 떨어져도 되는
	// 여유 사거리(무기 리치)"만 뜻하도록 바꿈 - AttackHitCheck에서 스윕 시작점을 자기 반경만큼
	// 밀어준 것과 같은 개념이라 서로 판정이 어긋나지 않음
	const float SelfRadius = AIPawn->GetAICollisionRadius();
	const float TargetRadius = Target->GetCapsuleComponent() ? Target->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
	float AttackRangeWithRadius = AIPawn->GetAIAttackRange() + SelfRadius + TargetRadius;

	bResult = (DistanceToTarget <= AttackRangeWithRadius);

	static TMap<TWeakObjectPtr<APawn>, bool> LastResultByPawn;
	bool& LastResult = LastResultByPawn.FindOrAdd(ControllingPawn);
	if (LastResult != bResult)
	{
		LastResult = bResult;
	}

	return bResult;
}

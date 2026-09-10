// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTTaskNode_TurnToTarget.h"
#include "CPAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../CPMonsterAIInterface.h"
#include "../../Player/CPPlayerCharacter.h"

UCPBTTaskNode_TurnToTarget::UCPBTTaskNode_TurnToTarget()
{
	NodeName = TEXT("TurnToTarget");
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

	ACPPlayerCharacter* TargetPawn = Cast<ACPPlayerCharacter>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BBKEY_TARGET));
	if (TargetPawn == nullptr) 
	{
		return EBTNodeResult::Failed;
	}
	
	FVector LookVector = TargetPawn->GetActorLocation() - ControllingPawn->GetActorLocation();
	LookVector.Z = 0.f;

	FRotator TargetRot = FRotationMatrix::MakeFromX(LookVector).Rotator();

	// 이 태스크는 BTTaskNode의 Tick(연속 실행) 없이 ExecuteTask 한 번으로 곧장 Succeeded를
	// 반환하는 "1회성" 태스크임. 그런데 RInterpTo는 그 한 프레임(DeltaSeconds)만큼만 회전을
	// 보간해주기 때문에, TurnSpeed(보간 계수)가 아무리 커도 실제로는 목표 각도 쪽으로 아주
	// 조금씩만 움직이고 끝나버림 - 이후 Attack 태스크가 바로 이어서 실행되므로 몬스터가 사실상
	// "예전 방향을 거의 그대로 유지한 채" 공격하는 셈이 됨.
	// 콜리전이 작은 근접 몬스터는 이동 중 CharacterMovement의 이동방향 정렬 덕분에 이미 플레이어
	// 쪽으로 얼추 향해 있어서 티가 안 났지만, 보스처럼 AttackRange(사거리)가 길면 아주 작은 각도
	// 오차도 스윕 끝에서는 큰 거리 오차로 벌어져서 계속 빗나갔음.
	// 이 태스크가 호출되는 시점은 곧바로 공격이 이어지는 타이밍이라, 부드럽게 보간할 이유가 없이
	// 그냥 목표 각도로 바로 맞춰줘야 함
	ControllingPawn->SetActorRotation(TargetRot);

	return EBTNodeResult::Succeeded;
}

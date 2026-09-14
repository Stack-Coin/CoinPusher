// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPTDecorator_ShouldRoar.h"
#include "AIController.h"
#include "Monster/Boss/CPMonsterBoss.h"

UCPTDecorator_ShouldRoar::UCPTDecorator_ShouldRoar()
{
	NodeName = TEXT("ShouldRoar");

	// 블랙보드 값 변경 이벤트가 없는 순수 C++ 조건이라, Observer Aborts가 실제로 작동하려면
	// 매 틱 직접 재확인해서 바뀐 경우에만 재평가를 요청해야 함 - TickNode 참고
	bNotifyTick = true;
}

bool UCPTDecorator_ShouldRoar::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return false;
	}

	ACPMonsterBoss* Boss = Cast<ACPMonsterBoss>(AIController->GetPawn());
	if (Boss == nullptr)
	{
		return false;
	}

	return Boss->ShouldRoar();
}

void UCPTDecorator_ShouldRoar::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 캐시된 이전 조건값과 지금 값을 비교해서 실제로 바뀐 경우에만 재평가 요청 - 매 틱 무조건
	// 요청하면 트리 전체를 매 프레임 재탐색하게 되어 낭비임(엔진 내장 틱 데코레이터들과 동일 패턴)
	FCPBTShouldRoarMemory* DecoratorMemory = CastInstanceNodeMemory<FCPBTShouldRoarMemory>(NodeMemory);
	const bool bResult = CalculateRawConditionValue(OwnerComp, NodeMemory);
	if (bResult != DecoratorMemory->bLastRawResult)
	{
		DecoratorMemory->bLastRawResult = bResult;
		OwnerComp.RequestExecution(this);
	}
}

uint16 UCPTDecorator_ShouldRoar::GetInstanceMemorySize() const
{
	return sizeof(FCPBTShouldRoarMemory);
}

void UCPTDecorator_ShouldRoar::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FCPBTShouldRoarMemory>(NodeMemory, InitType);
}

void UCPTDecorator_ShouldRoar::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FCPBTShouldRoarMemory>(NodeMemory, CleanupType);
}

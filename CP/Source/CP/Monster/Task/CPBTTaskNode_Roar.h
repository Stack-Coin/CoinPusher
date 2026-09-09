// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CPBTTaskNode_Roar.generated.h"

/**
 * 보스 전용. ACPMonsterBoss::RoarByAI()를 호출하고, 포효 몽타주가 끝날 때까지 InProgress로 대기.
 * CPBTTaskNode_Attack과 동일한 델리게이트 패턴(SetRoarDelegate + RoarByAI)을 사용.
 */
UCLASS()
class CP_API UCPBTTaskNode_Roar : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCPBTTaskNode_Roar();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

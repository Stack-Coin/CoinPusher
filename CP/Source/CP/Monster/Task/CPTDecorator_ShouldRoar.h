// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CPTDecorator_ShouldRoar.generated.h"

/**
 * 보스 전용. ACPMonsterBoss::ShouldRoar()가 true를 리턴하는 동안(체력이 임계치 밑 && 재무장 상태) true.
 * BT에서 이 데코레이터가 붙은 브랜치는 Observer Aborts: Self(또는 Lower Priority)로 설정해서
 * 추적/공격 중에도 조건이 참이 되는 즉시 끼어들 수 있게 구성할 것.
 */
UCLASS()
class CP_API UCPTDecorator_ShouldRoar : public UBTDecorator
{
	GENERATED_BODY()

public:
	UCPTDecorator_ShouldRoar();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

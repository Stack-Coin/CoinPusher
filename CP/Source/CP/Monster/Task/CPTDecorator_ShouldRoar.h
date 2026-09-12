// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CPTDecorator_ShouldRoar.generated.h"

/** 매 틱 CalculateRawConditionValue() 결과를 캐싱해뒀다가 바뀌었는지 비교하는 용도 - UE 엔진 자체
 *  네이티브 틱 데코레이터(BTDecorator_ConeCheck 등)가 쓰는 것과 동일한 패턴 */
struct FCPBTShouldRoarMemory
{
	bool bLastRawResult = false;
};

/**
 * 보스 전용. ACPMonsterBoss::ShouldRoar()가 true를 리턴하는 동안(체력이 임계치 밑 && 재무장 상태) true.
 * BT에서 이 데코레이터가 붙은 브랜치는 Observer Aborts: Self(또는 Lower Priority)로 설정해서
 * 추적/공격 중에도 조건이 참이 되는 즉시 끼어들 수 있게 구성할 것.
 *
 * 블랙보드 기반 데코레이터와 달리 이건 순수 C++ 조건이라, 값이 바뀌었다는 걸 엔진이 자동으로
 * 감지 못 함 - 그래서 Observer Aborts 옵션만 켜두는 걸로는 반응 안 하고, 매 틱 직접 재확인해서
 * 바뀐 경우에만 재평가를 요청해줘야 함(TickNode + bNotifyTick, FCPBTShouldRoarMemory로 이전 값 캐싱)
 */
UCLASS()
class CP_API UCPTDecorator_ShouldRoar : public UBTDecorator
{
	GENERATED_BODY()

public:
	UCPTDecorator_ShouldRoar();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTTaskNode_FindCloseNexus.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CPAI.h"
#include "AIController.h"
#include "Monster/CPMonsterWaypointArea.h"

UCPBTTaskNode_FindCloseNexus::UCPBTTaskNode_FindCloseNexus()
{
	NodeName = TEXT("FindCloseNexus");
}

EBTNodeResult::Type UCPBTTaskNode_FindCloseNexus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    APawn* ControllingPawn = Cast<APawn>(OwnerComp.GetAIOwner()->GetPawn());
    if (ControllingPawn == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
    {
        return EBTNodeResult::Failed;
    }

    // Nexus가 파괴됐을 경우, 웨이포인트로 이동
    // Nexus를 다시 찾는 건 UCPBTService_FindCloseNexus
    return FindRandomWaypoint(ControllingPawn, *Blackboard);
}

EBTNodeResult::Type UCPBTTaskNode_FindCloseNexus::FindRandomWaypoint(APawn* ControllingPawn, UBlackboardComponent& Blackboard) const
{
    ACPMonsterWaypointArea* Area = Cast<ACPMonsterWaypointArea>(UGameplayStatics::GetActorOfClass(GetWorld(), ACPMonsterWaypointArea::StaticClass()));

    if (!Area)
    {
        return EBTNodeResult::Failed;
    }

    FVector RandomPoint;
    if (!Area->GetRandomPointInArea(RandomPoint, ControllingPawn))
    {
        return EBTNodeResult::Failed;
    }

    // 더 이상 노릴 Nexus가 없으니 타겟 정보도 비워줌
    Blackboard.SetValueAsObject(BBKEY_NEXUS, nullptr);
    Blackboard.SetValueAsVector(BBKEY_PATROLPOS, RandomPoint);

    return EBTNodeResult::Succeeded;
}

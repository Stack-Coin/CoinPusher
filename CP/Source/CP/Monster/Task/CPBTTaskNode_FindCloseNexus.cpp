// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTTaskNode_FindCloseNexus.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CPAI.h"
#include "AIController.h"
#include "Nexus/CPNexus.h"

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

    TArray<AActor*> NexusActors;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ACPNexus::StaticClass(),
        NexusActors
    );

    ACPNexus* ClosestNexus = nullptr;
    float MinDistance = TNumericLimits<float>::Max();

    for (AActor* Actor : NexusActors)
    {
        ACPNexus* Nexus = Cast<ACPNexus>(Actor);
        if (!Nexus)
        {
            continue;
        }

        const float Distance = FVector::Distance(ControllingPawn->GetActorLocation(), Nexus->GetActorLocation());

        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            ClosestNexus = Nexus;
        }
    }

    if (!ClosestNexus)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
    {
        return EBTNodeResult::Failed;
    }

    Blackboard->SetValueAsObject(BBKEY_NEXUS, ClosestNexus);

    FVector PatrolPos = ClosestNexus->GetActorLocation();
    PatrolPos.Z = 0.0f;

    Blackboard->SetValueAsVector(BBKEY_PATROLPOS, PatrolPos);

    return EBTNodeResult::Succeeded;
}

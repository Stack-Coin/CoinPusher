// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTService_Detect.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/OverlapResult.h"
#include "Monster/CPMonsterAIInterface.h"
#include "Monster/Task/CPAI.h"

UCPBTService_Detect::UCPBTService_Detect()
{
	NodeName = TEXT("Detect");
	Interval = 1.0f;
}

void UCPBTService_Detect::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr) 
	{
		return;
	}

	FVector Center = ControllingPawn->GetActorLocation();
	UWorld* World = ControllingPawn->GetWorld();
	if (World == nullptr) 
	{
		return;
	}

	ICPMonsterAIInterface* AIPawn = Cast<ICPMonsterAIInterface>(ControllingPawn);
	if (AIPawn == nullptr) 
	{
		return;
	}

	float DetectRadius = AIPawn->GetAIDetectRange();

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(Detect), false, ControllingPawn);

	// todo. 공격 피격 협업
	bool bResult = World->OverlapMultiByChannel(
		OverlapResults,
		Center,
		FQuat::Identity,
		ECC_GameTraceChannel1, // todo. CCHANEL_CPACTION, Player에서는 Overlap, Monster에서는 Ignore
		FCollisionShape::MakeSphere(DetectRadius),
		CollisionQueryParams
	);

	if (bResult) 
	{
		for (auto const& OverlapResult : OverlapResults) 
		{
			APawn* Pawn = Cast<APawn>(OverlapResult.GetActor());
			if (Pawn && Pawn->GetController() && Pawn->GetController()->IsPlayerController()) 
			{
				OwnerComp.GetBlackboardComponent()->SetValueAsObject(BBKEY_TARGET, Pawn);

				DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Green, false, 0.2f);
				return;
			}
		}
	}

	OwnerComp.GetBlackboardComponent()->SetValueAsObject(BBKEY_TARGET, nullptr);

	DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Red, false, 0.2f);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTService_Detect.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/OverlapResult.h"
#include "Monster/CPMonsterAIInterface.h"
#include "Monster/Task/CPAI.h"
#include "../../Player/CPPlayerCharacter.h"
#include "Debug/CPDebugCollisionSubsystem.h"

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

	// UBTService nodes are shared across every AI running this behavior tree (no per-instance state), so
	// the F1 debug widget's MonsterDetectRange checkbox is read live here instead of cached in a member
	const UCPDebugCollisionSubsystem* DebugSubsystem = World->GetSubsystem<UCPDebugCollisionSubsystem>();
	const bool bDrawDebugDetectRange = DebugSubsystem && DebugSubsystem->IsCategoryVisible(ECPDebugCollisionCategory::MonsterDetectRange);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(Detect), false, ControllingPawn);

	// todo. 공격 피격 협업
	bool bResult = World->OverlapMultiByChannel(
		OverlapResults,
		Center,
		FQuat::Identity,
		ECC_GameTraceChannel1,
		FCollisionShape::MakeSphere(DetectRadius),
		CollisionQueryParams
	);

	if (bResult) 
	{
		for (auto const& OverlapResult : OverlapResults) 
		{
			ACPPlayerCharacter* Player = Cast<ACPPlayerCharacter>(OverlapResult.GetActor());

			if (Player && Player->GetController() && Player->GetController()->IsPlayerController()) 
			{
				OwnerComp.GetBlackboardComponent()->SetValueAsObject(BBKEY_TARGET, Player);

				if (bDrawDebugDetectRange)
				{
					DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Green, false, 0.2f);
				}
				return;
			}
		}
	}

	OwnerComp.GetBlackboardComponent()->SetValueAsObject(BBKEY_TARGET, nullptr);

	if (bDrawDebugDetectRange)
	{
		DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Red, false, 0.2f);
	}
}

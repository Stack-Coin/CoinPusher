// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Task/CPBTService_Detect.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
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

	UWorld* World = ControllingPawn->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 1인 플레이
	ACPPlayerCharacter* Player = Cast<ACPPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (Player != nullptr && Player->IsDowned())
	{
		Player = nullptr;
	}

	OwnerComp.GetBlackboardComponent()->SetValueAsObject(BBKEY_TARGET, Player);

	// 태준 Debug 코드
	const UCPDebugCollisionSubsystem* DebugSubsystem = World->GetSubsystem<UCPDebugCollisionSubsystem>();
	const bool bDrawDebugTargetLine = DebugSubsystem && DebugSubsystem->IsCategoryVisible(ECPDebugCollisionCategory::MonsterDetectRange);

	if (bDrawDebugTargetLine && Player)
	{
		DrawDebugLine(World, ControllingPawn->GetActorLocation(), Player->GetActorLocation(), FColor::Green, false, 0.2f, 0, 2.f);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/CPMonsterAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "Task/CPAI.h"

ACPMonsterAIController::ACPMonsterAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Data
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTAssetRef(TEXT("/Script/AIModule.BehaviorTree'/Game/Monster/AI/BT_Monster.BT_Monster'"));
	if (BTAssetRef.Object != nullptr)
	{
		MonsterBT = BTAssetRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBlackboardData> BBAssetRef(TEXT("/Script/AIModule.BlackboardData'/Game/Monster/AI/BB_Monster.BB_Monster'"));
	if (BBAssetRef.Object != nullptr) 
	{
		MonsterBB = BBAssetRef.Object;
	}
}

void ACPMonsterAIController::RunAI()
{
	UBlackboardComponent* BBComp = Blackboard.Get();
	if (UseBlackboard(MonsterBB, BBComp))
	{
		Blackboard->SetValueAsVector(BBKEY_SPAWNPOS, GetPawn()->GetActorLocation());

		bool bResult = RunBehaviorTree(MonsterBT);
		ensure(bResult);
	}
}

void ACPMonsterAIController::StopAI()
{
	StopMovement();
	SetActorEnableCollision(false);

	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComp)
	{
		BTComp->StopTree();
	}
}

void ACPMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RunAI();
}

void ACPMonsterAIController::OnUnPossess()
{
	StopAI();

	Super::OnUnPossess();
}

void ACPMonsterAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		StuckCheckTimer = 0.0f;
		return;
	}

	StuckCheckTimer += DeltaSeconds;
	if (StuckCheckTimer < StuckCheckInterval)
	{
		return;
	}
	StuckCheckTimer = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s Tick check at %s"), *GetNameSafe(MyPawn), *MyPawn->GetActorLocation().ToString());

	// 추적 중 NavMesh 영역을 완전히 벗어난 경우 우선 복귀시킴(TeleportTo로 위치가 바뀔 수 있으므로 stuck 체크보다 먼저)
	const bool bRecovered = RecoverIfOffNavMesh(MyPawn->GetActorLocation());
	if (bRecovered)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s recovered to %s"), *GetNameSafe(MyPawn), *MyPawn->GetActorLocation().ToString());
	}

	UPathFollowingComponent* PFComp = GetPathFollowingComponent();
	UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s PFComp=%s Status=%d"), *GetNameSafe(MyPawn), PFComp ? TEXT("valid") : TEXT("null"),
		PFComp ? static_cast<int32>(PFComp->GetStatus()) : -1);

	if (!PFComp || PFComp->GetStatus() != EPathFollowingStatus::Moving)
	{
		StuckCheckLastLocation = FVector::ZeroVector;
		return;
	}

	const FVector CurrentLocation = MyPawn->GetActorLocation();
	const float MovedDist = StuckCheckLastLocation.IsZero() ? -1.f : FVector::Dist(CurrentLocation, StuckCheckLastLocation);
	UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s MovedDist=%.1f Threshold=%.1f"), *GetNameSafe(MyPawn), MovedDist, StuckDistanceThreshold);

	if (!StuckCheckLastLocation.IsZero() && FVector::DistSquared(CurrentLocation, StuckCheckLastLocation) < FMath::Square(StuckDistanceThreshold))
	{
		// NavMesh 경계 등에 끼여 거의 안 움직이는 상태 - 이동 요청을 취소해 BT가 새 목적지로 재시도하게 함
		UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s STUCK -> StopMovement"), *GetNameSafe(MyPawn));
		StopMovement();
	}

	StuckCheckLastLocation = CurrentLocation;
}

bool ACPMonsterAIController::RecoverIfOffNavMesh(const FVector& CurrentLocation)
{
	APawn* MyPawn = GetPawn();
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!MyPawn || !NavSys)
	{
		return false;
	}

	FNavAgentProperties AgentProps;
	if (const INavAgentInterface* NavAgent = Cast<INavAgentInterface>(MyPawn))
	{
		AgentProps = NavAgent->GetNavAgentPropertiesRef();
	}

	FNavLocation OnMeshLocation;
	const bool bOnNavMesh = NavSys->ProjectPointToNavigation(CurrentLocation, OnMeshLocation,
		FVector(OffNavMeshCheckExtentXY, OffNavMeshCheckExtentXY, OffNavMeshCheckExtentZ), &AgentProps);
	UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s OnNavMeshCheck=%s (extent %.0f/%.0f, agent radius=%.0f height=%.0f)"),
		*GetNameSafe(MyPawn), bOnNavMesh ? TEXT("true") : TEXT("false"),
		OffNavMeshCheckExtentXY, OffNavMeshCheckExtentZ, AgentProps.AgentRadius, AgentProps.AgentHeight);
	if (bOnNavMesh)
	{
		return false;
	}

	FNavLocation RecoveryLocation;
	const bool bFoundRecovery = NavSys->ProjectPointToNavigation(CurrentLocation, RecoveryLocation,
		FVector(OffNavMeshRecoveryExtentXY, OffNavMeshRecoveryExtentXY, OffNavMeshRecoveryExtentZ), &AgentProps);
	UE_LOG(LogTemp, Warning, TEXT("[NavStuckDebug] %s off-mesh! RecoverySearch=%s target=%s"),
		*GetNameSafe(MyPawn), bFoundRecovery ? TEXT("found") : TEXT("NOT FOUND"), *RecoveryLocation.Location.ToString());

	if (bFoundRecovery)
	{
		// NavMesh 밖으로 나간 상태 - 가장 가까운 유효 지점으로 강제 복귀
		MyPawn->TeleportTo(RecoveryLocation.Location, MyPawn->GetActorRotation());
		StopMovement();
		return true;
	}

	return false;
}

bool ACPMonsterAIController::TryRecoverFromOffNavMesh()
{
	APawn* MyPawn = GetPawn();
	return MyPawn ? RecoverIfOffNavMesh(MyPawn->GetActorLocation()) : false;
}

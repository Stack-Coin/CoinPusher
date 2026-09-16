// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CPMonsterAIController.generated.h"

/**
 * 
 */
UCLASS()
class CP_API ACPMonsterAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ACPMonsterAIController();

public:
	void RunAI();
	void StopAI();

	void SetTargetActor(AActor* NewTarget);
	AActor* GetTargetActor() const;

	// 현재 Pawn 위치가 NavMesh를 벗어났으면 가장 가까운 지점으로 복귀시킴. AI Move To 실패(On Fail) 시 즉시 호출용.
	bool TryRecoverFromOffNavMesh();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	// Data
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> MonsterBT;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBlackboardData> MonsterBB;

	// NavMesh 경계 등에서 물리적으로 끼여 위치가 거의 안 바뀌는데 Moving 상태만 유지되는 경우, 이동 요청을 취소해 BT가 새 목적지를 다시 잡게 함
	UPROPERTY(EditDefaultsOnly, Category = "AI|Stuck Detection")
	float StuckCheckInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Stuck Detection")
	float StuckDistanceThreshold = 15.0f;

	FVector StuckCheckLastLocation = FVector::ZeroVector;
	float StuckCheckTimer = 0.0f;

	// 추적 중 NavMesh 영역을 완전히 벗어난 경우, 가장 가까운 NavMesh 지점으로 강제 복귀시킴
	UPROPERTY(EditDefaultsOnly, Category = "AI|Stuck Detection")
	float OffNavMeshCheckExtentXY = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Stuck Detection")
	float OffNavMeshCheckExtentZ = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Stuck Detection")
	float OffNavMeshRecoveryExtentXY = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Stuck Detection")
	float OffNavMeshRecoveryExtentZ = 500.0f;

	bool RecoverIfOffNavMesh(const FVector& CurrentLocation);
};

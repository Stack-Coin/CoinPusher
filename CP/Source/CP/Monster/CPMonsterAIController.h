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

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	// Data
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> MonsterBT;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBlackboardData> MonsterBB;
};

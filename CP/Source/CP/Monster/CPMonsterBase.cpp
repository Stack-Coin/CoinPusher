// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/CPMonsterBase.h"
#include "Monster/CPMonsterAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACPMonsterBase::ACPMonsterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = ACPMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void ACPMonsterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACPMonsterBase::AttackHitCheck()
{
	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_None, false, this);
	bool bResult = GetWorld()->SweepSingleByChannel
	(
		HitResult,
		GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * GetAIAttackRange(),
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel2, // todo. 코인 푸셔 채널 파기
		FCollisionShape::MakeSphere(10.f),
		Params
	);

	if (bResult)
	{
		if (HitResult.GetActor()->IsValidLowLevel())
		{
			UGameplayStatics::ApplyDamage(HitResult.GetActor(), 50.0f, GetController(), this, UDamageType::StaticClass());
		}
	}
}

void ACPMonsterBase::Dead()
{
}

float ACPMonsterBase::GetAIPatrolRadius()
{
	return 800.0f;
}

float ACPMonsterBase::GetAIDetectRange()
{
	return 400.0f;
}

float ACPMonsterBase::GetAIAttackRange()
{
	// todo. Stat Component에서 구하기

	return 100.0f;
}

float ACPMonsterBase::GetAITurnSpeed()
{
	return 0.0f;
}

void ACPMonsterBase::SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished)
{
	OnAttackFinished = InOnAttackFinished;
}

void ACPMonsterBase::AttackByAI()
{
	// todo. 공격
}

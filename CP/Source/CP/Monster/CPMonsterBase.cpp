// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/CPMonsterBase.h"
#include "Monster/CPMonsterAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "BrainComponent.h"

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

	GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;
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
		ECollisionChannel::ECC_GameTraceChannel2, // todo. 코인 푸셔 및 캐릭터 채널 파기
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
	if (bIsDead)
	{
		OnAttackFinished.ExecuteIfBound();
		return;
	}

	bIsDead = true;

	// 사망 후 BT가 공격 몽타주를 다시 재생하지 못하게 정지
	if (ACPMonsterAIController* AIController = GetController<ACPMonsterAIController>())
	{
		AIController->StopAI();
	}

	// todo. 임시로 아이템 드랍
	if (GetWorld() && CoinItem)
	{
		FVector Location = GetActorLocation();
		Location.X += 100.f;
		FRotator Rotation = GetActorRotation();

		FActorSpawnParameters SpawnParms;
		SpawnParms.Owner = this;
		SpawnParms.Instigator = GetInstigator();

		ACPCoinItem* SpawnActor = GetWorld()->SpawnActor<ACPCoinItem>(CoinItem, Location, Rotation, SpawnParms);
	}

	OnMonsterDied.Broadcast();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) && IsValid(DeadMontage))
	{
		AnimInstance->StopAllMontages(0.0f);

		const float Duration = AnimInstance->Montage_Play(DeadMontage);
		if (Duration > 0.0f)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindLambda(
				[this](UAnimMontage*, bool)
				{
					SetLifeSpan(2.0f);
				});

			AnimInstance->Montage_SetEndDelegate(EndDelegate, DeadMontage);

			return;
		}
	}

	SetLifeSpan(2.0f);
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
	return 200.0f;
}

float ACPMonsterBase::GetAITurnSpeed()
{
	return 2.0f;
}

void ACPMonsterBase::SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished)
{
	OnAttackFinished = InOnAttackFinished;
}

void ACPMonsterBase::AttackByAI()
{
	TObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AttackMontage)
	{
		AnimInstance->StopAllMontages(0.0f);
		AnimInstance->Montage_Play(AttackMontage, 1.0f);

		FOnMontageEnded MontageEndDelegate;
		MontageEndDelegate.BindUObject(this, &ACPMonsterBase::NotifyAttackActionEnd);

		AnimInstance->Montage_SetEndDelegate(MontageEndDelegate, AttackMontage);
	}
}

float ACPMonsterBase::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0) 
	{
		Dead();
	}
	else if (IsValid(DamageCauser)) 
	{
		FVector KnockbackDirection = GetActorLocation() - DamageCauser->GetActorLocation();
		KnockbackDirection.Z = 0.f;
		KnockbackDirection.Normalize();

		LaunchCharacter(KnockbackDirection * 300.0f + FVector::UpVector * 300.0f, true, true);
	}

	return DamageAmount;
}

void ACPMonsterBase::NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted)
{
	OnAttackFinished.ExecuteIfBound();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/CPMonsterBase.h"
#include "Monster/CPMonsterAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"

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
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeadMontage)
	{
		AnimInstance->StopAllMontages(0.0f);
		AnimInstance->Montage_Play(DeadMontage, 1.0f);
	}

	SetActorEnableCollision(false);

	// todo. 임시로 아이템 드랍
	{
		FVector SpawnLocation = GetActorLocation();
		SpawnLocation.Z += 50.0f;

		AStaticMeshActor* Item = GetWorld()->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(),
			SpawnLocation,
			FRotator::ZeroRotator
		);

		if (Item)
		{
			UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(
				nullptr,
				TEXT("/Engine/BasicShapes/Sphere.Sphere")
			);

			Item->GetStaticMeshComponent()->SetStaticMesh(SphereMesh);
			Item->SetActorScale3D(FVector(0.25f));
		}
	}

	// todo. 몬스터가 죽고 나서 이후에 델리게이트로 송신 후 시간 딜레이를 주고 Item Spawner를 사용해서 코인을 Spawn하는 걸로
	FTimerHandle DeadTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		DeadTimerHandle,
		FTimerDelegate::CreateLambda([&]()
			{
				Destroy();
			}
		),
		DeadMontage->GetPlayLength(),
		false
	);
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
	return 100.0f;
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

		LaunchCharacter(KnockbackDirection * 200.0f + FVector::UpVector * 150.0f, true, true);
	}

	return DamageAmount;
}

void ACPMonsterBase::NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted)
{
	OnAttackFinished.ExecuteIfBound();
}

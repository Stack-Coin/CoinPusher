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

	StatComponent = CreateDefaultSubobject<UCPMonsterStatComponent>(TEXT("StatComponent"));
}

// Called when the game starts or when spawned
void ACPMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	if (StatComponent)
	{
		StatComponent->InitStat(ECPMonsterType::Normal, 1);
	}

	GetCharacterMovement()->MaxWalkSpeed = GetAIMoveSpeed();

	if (Collider)
	{
		Collider->SetCapsuleRadius(GetAICollisionRadius());
	}
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
		ECollisionChannel::ECC_GameTraceChannel1, // todo. 코인 푸셔 및 캐릭터 채널 파기
		FCollisionShape::MakeSphere(10.f),
		Params
	);

	if (bResult)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->IsValidLowLevel())
		{
			UGameplayStatics::ApplyDamage(HitActor, GetAIAttackPower(), GetController(), this, UDamageType::StaticClass());

			// KnockbackPower 스탯만큼 맞은 대상을 밀어냄 (ICPKnockbackable을 구현한 대상만)
			if (ICPKnockbackable* KnockbackTarget = Cast<ICPKnockbackable>(HitActor))
			{
				KnockbackTarget->ApplyKnockback(GetActorForwardVector(), GetAIKnockbackPower(), this);
			}
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
		SpawnParms.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ACPCoin* SpawnActor = GetWorld()->SpawnActor<ACPCoin>(CoinItem, Location, Rotation, SpawnParms);

		if (SpawnActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawn Success: %s"), *SpawnActor->GetName());
		}
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

	if (StatComponent)
	{
		StatComponent->CurrentHealth -= DamageAmount;

		if (StatComponent->CurrentHealth <= 0.0f)
		{
			Dead();
		}
	}

	return DamageAmount;
}

void ACPMonsterBase::ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	// KnockbackDuration/KnockbackLaunchStrength 프로퍼티가 제거되어, 기존에 쓰던 기본값을 그대로 리터럴로 사용
	// (필요하면 나중에 별도 프로퍼티나 DefaultStat 쪽으로 다시 옮길 수 있음)
	ApplyCPKnockbackToCharacter(this, Direction, Distance, 0.2f, 1000.0f);
}

void ACPMonsterBase::NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted)
{
	OnAttackFinished.ExecuteIfBound();
}

// 몬스터 스텟 컴포넌트
UCPMonsterStatComponent* ACPMonsterBase::GetAIStatComponent() const
{
	return StatComponent;
}

// 몬스터 템플릿
ECPMonsterMoveType ACPMonsterBase::GetAIMoveType()
{
	return StatComponent ? StatComponent->MonsterTemplete.MoveType : ECPMonsterMoveType::Walking;
}

ECPMonsterAttackType ACPMonsterBase::GetAIAttackType()
{
	return StatComponent ? StatComponent->MonsterTemplete.AttackType : ECPMonsterAttackType::Melee;
}

FCPMonsterProjectileStat ACPMonsterBase::GetAIProjectileStat()
{
	return StatComponent ? StatComponent->MonsterTemplete.ProjectileStat : FCPMonsterProjectileStat();
}

// 웨이브에 따른 수치 변화 있음
float ACPMonsterBase::GetAIMaxHealth()
{
	return StatComponent ? StatComponent->MaxHealth : 0.0f;
}

float ACPMonsterBase::GetAICurrentHealth()
{
	return StatComponent ? StatComponent->CurrentHealth : 0.0f;
}

float ACPMonsterBase::GetAIMoveSpeed()
{
	return StatComponent ? StatComponent->MoveSpeed : 0.0f;
}

float ACPMonsterBase::GetAIAttackPower()
{
	return StatComponent ? StatComponent->AttackPower : 0.0f;
}

// 웨이브에 따른 수치 변화 없음
float ACPMonsterBase::GetAIAttackSpeed()
{
	return StatComponent ? StatComponent->DefaultStat.AttackSpeed : 1.0f;
}

float ACPMonsterBase::GetAIKnockbackPower()
{
	return StatComponent ? StatComponent->DefaultStat.KnockbackPower : 0.0f;
}

float ACPMonsterBase::GetAIDetectRange()
{
	return StatComponent ? StatComponent->DefaultStat.DetectRange : 0.0f;
}

float ACPMonsterBase::GetAICollisionRadius()
{
	return StatComponent ? StatComponent->DefaultStat.CollisionRadius : 0.0f;
}

float ACPMonsterBase::GetAIPatrolRadius()
{
	return StatComponent ? StatComponent->DefaultStat.PatrolRadius : 0.0f;
}

float ACPMonsterBase::GetAIAttackRange()
{
	return StatComponent ? StatComponent->DefaultStat.AttackRange : 0.0f;
}

float ACPMonsterBase::GetAITurnSpeed()
{
	return StatComponent ? StatComponent->DefaultStat.TurnSpeed : 0.0f;
}
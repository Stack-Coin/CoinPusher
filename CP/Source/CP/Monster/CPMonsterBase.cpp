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
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Debug/CPDebugCollisionShapeComponent.h"
#include "Debug/CPDebugCollisionSubsystem.h"

// Sets default values
ACPMonsterBase::ACPMonsterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = ACPMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	StatComponent = CreateDefaultSubobject<UCPMonsterStatComponent>(TEXT("StatComponent"));

	DebugHitboxShape = CreateDefaultSubobject<UCPDebugCollisionShapeComponent>(TEXT("DebugHitboxShape"));
	DebugHitboxShape->Category = ECPDebugCollisionCategory::EnemyHitbox;
	DebugHitboxShape->ShapeColor = FColor::Orange;
	DebugHitboxShape->SetTargetComponent(GetCapsuleComponent());
}

// Called when the game starts or when spawned
void ACPMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	if (StatComponent)
	{
		StatComponent->InitStat(MonsterType, 1);
	}

	GetCharacterMovement()->MaxWalkSpeed = GetAIMoveSpeed();

	if (Collider)
	{
		Collider->SetCapsuleRadius(GetAICollisionRadius());
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseRVOAvoidance = true;
		MoveComp->AvoidanceConsiderationRadius = GetAICollisionRadius() * 3.f;
		MoveComp->AvoidanceWeight = 0.5f;

		// 몬스터끼리만 서로 피하도록 그룹 마스크 설정
		MoveComp->SetAvoidanceGroup(1);
		MoveComp->SetGroupsToAvoid(1);
	}

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &ACPMonsterBase::HandleDebugCollisionVisibilityChanged);
		bDrawDebugAttackRange = Subsystem->IsCategoryVisible(ECPDebugCollisionCategory::MonsterAttackRange);
	}
}

void ACPMonsterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsDead)
	{
		SeparateFromOtherMonsters(DeltaSeconds);
	}
}

void ACPMonsterBase::HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (Category == ECPDebugCollisionCategory::MonsterAttackRange)
	{
		bDrawDebugAttackRange = bVisible;
	}
}

void ACPMonsterBase::AttackHitCheck()
{
	const FVector SweepStart = GetActorLocation();
	const FVector SweepEnd = SweepStart + GetActorForwardVector() * GetAIAttackRange();
	constexpr float SweepRadius = 10.f;

	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_None, false, this);
	bool bResult = GetWorld()->SweepSingleByChannel
	(
		HitResult,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel1, // todo. 코인 푸셔 및 캐릭터 채널 파기
		FCollisionShape::MakeSphere(SweepRadius),
		Params
	);

	if (bDrawDebugAttackRange)
	{
		// Visualizes the swept sphere (Start->End, radius SweepRadius) as the equivalent capsule
		const FVector Center = (SweepStart + SweepEnd) * 0.5f;
		const float HalfHeight = (SweepEnd - SweepStart).Size() * 0.5f + SweepRadius;
		const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(GetActorForwardVector()).ToQuat();
		DrawDebugCapsule(GetWorld(), Center, HalfHeight, SweepRadius, CapsuleRotation, bResult ? FColor::Red : FColor::Orange, false, 0.5f, 0, 1.5f);
	}

	if (bResult)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->IsValidLowLevel())
		{
			UGameplayStatics::ApplyDamage(HitActor, GetAIAttackPower(), GetController(), this, UDamageType::StaticClass());

			// KnockbackPower 스탯만큼 맞은 대상을 밀어냄 (ICPKnockbackable을 구현한 대상만)
			if (ICPKnockbackable* KnockbackTarget = Cast<ICPKnockbackable>(HitActor))
			{
				KnockbackTarget->ApplyKnockback(GetActorForwardVector(), GetAIKnockbackDistance(), this);
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

		ACPCoinItem* SpawnActor = GetWorld()->SpawnActor<ACPCoinItem>(CoinItem, Location, Rotation, SpawnParms);

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

void ACPMonsterBase::SeparateFromOtherMonsters(float DeltaSeconds)
{
	const float MyRadius = GetAICollisionRadius();
	if (MyRadius <= 0.f)
	{
		return;
	}

	// 콜리전 반경 + 여유 간격보다 살짝 넓게 잡아서 그 범위 안에 있는 다른 몬스터를 찾음
	const float SearchRadius = (MyRadius + SeparationPadding) * 2.5f;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(NAME_None, false, this);
	GetWorld()->OverlapMultiByObjectType
	(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(SearchRadius),
		Params
	);

	FVector PushDirection = FVector::ZeroVector;
	int32 NeighborCount = 0;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACPMonsterBase* Other = Cast<ACPMonsterBase>(Overlap.GetActor());
		if (!Other || Other == this || Other->bIsDead)
		{
			continue;
		}

		// 수평을 기준으로 실제 몬스터 간의 거리 판단
		FVector Delta = GetActorLocation() - Other->GetActorLocation();
		Delta.Z = 0.f;
		const float Distance = Delta.Size();

		// 콜리전 반경끼리 딱 닿기 전에 SeparationPadding만큼 여유를 두고 몬스터 간의 거리 판단
		const float MinDistance = MyRadius + Other->GetAICollisionRadius() + SeparationPadding;

		if (Distance < MinDistance && Distance > KINDA_SMALL_NUMBER)
		{
			// 겹친 비율이 클수록(가까울수록) 세게 밀어냄
			const float W = (MinDistance - Distance) / MinDistance;
			PushDirection += Delta.GetSafeNormal() * W;
			++NeighborCount;
		}
	}

	if (NeighborCount > 0)
	{
		//정규화하면 항상 같은 세기로만 밀려나서, 몬스터 군집에서는 힘이 서로 상쇄됨. 
		// 정규화하지 않고 최대 1.5배로만 클램프로, 많이 겹칠수록 더 세게 밀려나도록 함.
		const FVector PushVector = PushDirection.GetClampedToMaxSize(1.5f);
		AddActorWorldOffset(PushVector * SeparationSpeed * DeltaSeconds, true);
	}
}

// 몬스터 스텟 컴포넌트
UCPMonsterStatComponent* ACPMonsterBase::GetAIStatComponent() const
{
	return StatComponent;
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

float ACPMonsterBase::GetAIKnockbackDistance()
{
	return StatComponent ? StatComponent->DefaultStat.KnockbackDistance : 0.0f;
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

float ACPMonsterBase::GetAIMoveAcceptableRadius()
{
	return StatComponent ? StatComponent->DefaultStat.MoveAcceptableRadius : 0.0f;
}
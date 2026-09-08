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
#include "TimerManager.h"
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
		MoveComp->AvoidanceConsiderationRadius = GetAICollisionRadius() * GetAIAvoidanceRadiusMultiplier();
		MoveComp->AvoidanceWeight = GetAIAvoidanceWeight();

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

void ACPMonsterBase::ApplyWaveStat(int32 InWave)
{
	if (StatComponent)
	{
		StatComponent->InitStat(MonsterType, InWave);
	}

	// StatComponent가 갱신한 MoveSpeed를 실제 이동 속도에도 반영
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = GetAIMoveSpeed();
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
		ECollisionChannel::ECC_GameTraceChannel1,
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

	// 사망 후에는 다른 액터와 전혀 부딪히지 않도록 콜리전을 완전히 끔
	if (Collider)
	{
		Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (MonsterMesh)
	{
		MonsterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 콜리전이 사라지면 무브먼트가 바닥 참조를 잃어 사망 모션 중 파묻힐 수 있어 무브먼트도 함께 정지
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

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
					SetLifeSpan(1.0f);
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
	const float CurrentHealth = GetAICurrentHealth();

	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Knockback Blocked (dead) | Health=%.1f Distance(param)=%.1f"), *GetName(), CurrentHealth, Distance);
		return;
	}

	// 수평 방향만 사용
	FVector FlatDirection = Direction;
	FlatDirection.Z = 0.0f;

	if (!FlatDirection.Normalize())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Knockback Blocked (direction) | Health=%.1f Distance(param)=%.1f"), *GetName(), CurrentHealth, Distance);
		return;
	}

	// 몬스터 타입별로 조절 가능
	const float KnockbackDuration = GetAIKnockbackDuration();
	const float Speed = KnockbackDuration > 0.0f ? (Distance / KnockbackDuration) : Distance;

	// TEST
	//constexpr float TestKnockbackDistance = 300.0f;
	//const float Speed = KnockbackDuration > 0.0f ? (TestKnockbackDistance / KnockbackDuration) : TestKnockbackDistance;

	// 이중 넉백 방지는 Z(수직)에만 적용
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const bool bWithinZCooldown = LastKnockbackTime >= 0.f && CurrentTime - LastKnockbackTime < KnockbackReapplyCooldown;

	const float CurrentZVelocity = GetVelocity().Z;
	float NewZVelocity = CurrentZVelocity;

	if (bWithinZCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Knockback Z Blocked (cooldown) | Health=%.1f Distance(param)=%.1f"), *GetName(), CurrentHealth, Distance);
	}
	else
	{
		LastKnockbackTime = CurrentTime;
		// 지금 갖고 있는 수직 속도 위에 KnockbackPower만큼 얹되, MaxKnockbackZVelocity를 넘지 않게 clamp
		// (이미 공중에 떠 있는 상태에서 또 맞아도 무한정 높이 올라가지 않도록)
		NewZVelocity = FMath::Min(CurrentZVelocity + GetAIKnockbackPower(), MaxKnockbackZVelocity);

		UE_LOG(LogTemp, Warning, TEXT("[%s] Knockback Z Applied | Health=%.1f Distance(param)=%.1f NewZ=%.1f"), *GetName(), CurrentHealth, Distance, NewZVelocity);
	}

	// LaunchCharacter는 호출 즉시 MovementMode를 바꾸는 게 아니라, PendingLaunchVelocity를 예약만 해두고
	// 실제 속도 적용 + MOVE_Falling 강제 전환은 "다음 무브먼트 틱"에 HandlePendingLaunch()가 처리함.
	// 그래서 호출 전에 미리 Flying 여부를 저장해두고, 한 틱 늦춰서(SetTimerForNextTick) 복구해야 함
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const bool bWasFlying = MoveComp && MoveComp->MovementMode == MOVE_Flying;

	const FVector LaunchVelocity = FlatDirection * Speed + FVector(0.f, 0.f, NewZVelocity);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Knockback Launch | Health=%.1f Distance(param)=%.1f Speed=%.1f Launch=%s | MoveMode=%d Flying=%d"),
		*GetName(), CurrentHealth, Distance, Speed,
		*LaunchVelocity.ToString(),
		MoveComp ? (int32)MoveComp->MovementMode.GetValue() : -1, bWasFlying ? 1 : 0);

	LaunchCharacter(LaunchVelocity, true, true);

	if (bWasFlying)
	{
		TWeakObjectPtr<ACPMonsterBase> WeakThis(this);
		GetWorldTimerManager().SetTimerForNextTick([WeakThis]()
			{
				ACPMonsterBase* Monster = WeakThis.Get();
				if (!Monster || Monster->bIsDead)
				{
					return;
				}

				if (UCharacterMovementComponent* MC = Monster->GetCharacterMovement())
				{
					// HandlePendingLaunch가 이 시점에는 이미 Falling으로 바꿔놓은 상태이므로, 그걸 다시 Flying으로 되돌림
					if (MC->MovementMode == MOVE_Falling)
					{
						MC->SetMovementMode(MOVE_Flying);
					}
				}
			});
	}
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

	const float MySeparationPadding = GetAISeparationPadding();

	// 콜리전 반경 + 여유 간격보다 살짝 넓게 잡아서 그 범위 안에 있는 다른 몬스터를 찾음
	const float SearchRadius = (MyRadius + MySeparationPadding) * 2.5f;

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
		const float MinDistance = MyRadius + Other->GetAICollisionRadius() + MySeparationPadding;

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
		AddActorWorldOffset(PushVector * GetAISeparationSpeed() * DeltaSeconds, true);
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

float ACPMonsterBase::GetAIKnockbackPower()
{
	return StatComponent ? StatComponent->DefaultStat.KnockbackPower : 250.0f;
}

float ACPMonsterBase::GetAIKnockbackDuration()
{
	return StatComponent ? StatComponent->DefaultStat.KnockbackDuration : 0.2f;
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

// 군중 제어(RVO 회피 / 몬스터 간 분리)
float ACPMonsterBase::GetAIAvoidanceRadiusMultiplier()
{
	return StatComponent ? StatComponent->DefaultStat.AvoidanceRadiusMultiplier : 3.0f;
}

float ACPMonsterBase::GetAIAvoidanceWeight()
{
	return StatComponent ? StatComponent->DefaultStat.AvoidanceWeight : 0.5f;
}

float ACPMonsterBase::GetAISeparationPadding()
{
	return StatComponent ? StatComponent->DefaultStat.SeparationPadding : 70.0f;
}

float ACPMonsterBase::GetAISeparationSpeed()
{
	return StatComponent ? StatComponent->DefaultStat.SeparationSpeed : 400.0f;
}
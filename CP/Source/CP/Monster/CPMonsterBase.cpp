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
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

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

	GetCapsuleComponent()->SetCapsuleRadius(GetAICollisionRadius());


	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// RVO 회피 반경 배율 / 비중은 몬스터마다 다르게 줄 이유가 없어서 데이터테이블에서 빼고
		// 코드 상 권장값으로 고정함 (AvoidanceWeight는 0~1 사이 값이어야 함)
		constexpr float RVOAvoidanceRadiusMultiplier = 3.f;
		constexpr float RVOAvoidanceWeight = 0.5f;

		MoveComp->bUseRVOAvoidance = true;
		MoveComp->AvoidanceConsiderationRadius = GetAICollisionRadius() * RVOAvoidanceRadiusMultiplier;
		MoveComp->AvoidanceWeight = RVOAvoidanceWeight;

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
	AddCCState(ECPMonsterCCState::Dead);

	// 사망 후에는 다른 액터와 전혀 부딪히지 않도록 콜리전을 완전히 끔
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
	PlayAttackMontage(AttackMontage);
}

void ACPMonsterBase::PlayAttackMontage(UAnimMontage* Montage)
{
	TObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && Montage)
	{

		AddCCState(ECPMonsterCCState::Attacking);

		AnimInstance->StopAllMontages(0.0f);
		AnimInstance->Montage_Play(Montage, 1.0f);

		FOnMontageEnded MontageEndDelegate;
		MontageEndDelegate.BindUObject(this, &ACPMonsterBase::NotifyAttackActionEnd);

		AnimInstance->Montage_SetEndDelegate(MontageEndDelegate, Montage);
	}
	else
	{
	}
}

float ACPMonsterBase::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 포효 등으로 무적 상태면 데미지 무시
	if (HasCCState(ECPMonsterCCState::Invulnerable))
	{
		return 0.f;
	}

	if (StatComponent)
	{
		StatComponent->CurrentHealth -= DamageAmount;

		// 체력이 0 이하여도 바로 죽이지 않고, 공격자가 TakeDamage 직후 별도로 거는 ApplyKnockback이
		// 먼저 재생될 시간(KnockbackDuration)을 준 다음에 실제 Dead()를 호출함
		if (!bIsDead && !bPendingDeath && StatComponent->CurrentHealth <= 0.0f)
		{
			bPendingDeath = true;

			TWeakObjectPtr<ACPMonsterBase> WeakThis(this);
			FTimerHandle DeathTimerHandle;
			GetWorldTimerManager().SetTimer(DeathTimerHandle, [WeakThis]()
			{
				if (ACPMonsterBase* StrongThis = WeakThis.Get())
				{
					StrongThis->Dead();
				}
			}, KnockbackDuration, false);
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

	FVector FlatDirection = Direction;
	FlatDirection.Z = 0.0f;

	if (!FlatDirection.Normalize())
	{
		return;
	}

	AddCCState(ECPMonsterCCState::Knockback);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const bool bWasFlying = MoveComp && MoveComp->MovementMode == MOVE_Flying;

	// Distance(밀려나는 거리)를 KnockbackDuration(밀려나는 데 걸리는 시간) 안에 이동하도록 속도로 환산
	const float Speed = Distance / KnockbackDuration;

	// Test
	//const float Speed = 1000.f / KnockbackDuration;

	const FVector LaunchVelocity = FlatDirection * Speed;

	// RVO 끄기
	if (MoveComp)
	{
		MoveComp->bUseRVOAvoidance = false;
	}

	// Flying/Ranged 몬스터는 RVO를 꺼도 BT의 MoveTo가 매 틱 이동 명령을 내려서 velocity가 되돌아감 ->
	// 넉백 동안은 AI 이동 명령 자체를 PauseMove로 멈추고, 끝나면 같은 요청을 ResumeMove로 재개
	FAIRequestID PausedMoveRequestID = FAIRequestID::InvalidRequest;
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (UPathFollowingComponent* PFC = AICon->GetPathFollowingComponent())
		{
			PausedMoveRequestID = PFC->GetCurrentRequestId();
			PFC->PauseMove(PausedMoveRequestID);
		}
	}

	LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/false);

	// KnockbackDuration 후 RVO 회피를 복구하고 Flying 복구
	TWeakObjectPtr<ACPMonsterBase> WeakThis(this);
	FTimerHandle RestoreHandle;
	GetWorldTimerManager().SetTimer(RestoreHandle, [WeakThis, bWasFlying, PausedMoveRequestID]()
	{
		if (ACPMonsterBase* StrongThis = WeakThis.Get())
		{
			if (UCharacterMovementComponent* InnerMoveComp = StrongThis->GetCharacterMovement())
			{
				InnerMoveComp->bUseRVOAvoidance = true;
				if (bWasFlying)
				{
					InnerMoveComp->SetMovementMode(MOVE_Flying);
				}
			}

			// 넉백 동안 멈춰뒀던 AI 이동 명령(PathFollowing)을 같은 요청 그대로 재개함
			if (AAIController* InnerAICon = Cast<AAIController>(StrongThis->GetController()))
			{
				if (UPathFollowingComponent* InnerPFC = InnerAICon->GetPathFollowingComponent())
				{
					InnerPFC->ResumeMove(PausedMoveRequestID);
				}
			}

			// CC 상태 비트에서 Knockback 해제
			StrongThis->RemoveCCState(ECPMonsterCCState::Knockback);
		}
	}, KnockbackDuration, false);
}

void ACPMonsterBase::NotifyAttackActionEnd(UAnimMontage* Montage, bool bInterrupted)
{
	RemoveCCState(ECPMonsterCCState::Attacking);

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

float ACPMonsterBase::GetAICollisionRadius()
{
	return StatComponent ? StatComponent->DefaultStat.CollisionRadius : 0.0f;
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

// 몬스터 간 분리
float ACPMonsterBase::GetAISeparationPadding()
{
	return StatComponent ? StatComponent->DefaultStat.SeparationPadding : 70.0f;
}

float ACPMonsterBase::GetAISeparationSpeed()
{
	return StatComponent ? StatComponent->DefaultStat.SeparationSpeed : 400.0f;
}
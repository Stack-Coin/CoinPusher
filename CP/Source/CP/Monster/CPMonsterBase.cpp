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

	// DataTable에 Half Height 값이 채워져 있으면 그걸로 캡슐 높이도 맞춤 (0이면 아직 데이터가
	// 안 채워진 것으로 보고 BP에 세팅된 기존 캡슐 Half Height를 그대로 둠)
	if (GetAICollisionHalfHeight() > 0.f)
	{
		GetCapsuleComponent()->SetCapsuleHalfHeight(GetAICollisionHalfHeight());
	}

	// 몬스터 타입마다 캡슐 Half Height가 달라서, 메쉬가 고정 오프셋으로 붙어있으면 캡슐 바닥과
	// 안 맞아 스폰 시 붕 뜨거나 파묻힌 것처럼 보일 수 있음 - 메쉬 Z를 캡슐 크기에 맞춰 정렬
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		FVector MeshRelativeLocation = MeshComp->GetRelativeLocation();
		MeshRelativeLocation.Z = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		MeshComp->SetRelativeLocation(MeshRelativeLocation);
	}

	// [임시 디버그] 스폰 직후 DataTable에서 실제로 어떤 수치가 들어왔는지 한 번에 확인용
	UE_LOG(LogTemp, Warning,
		TEXT("[임시 디버그] %s BeginPlay 스탯 - MonsterType=%d, MaxHealth=%.1f, MoveSpeed=%.1f, AttackPower=%.1f, AttackRange=%.1f, CollisionRadius=%.1f(실제 캡슐=%.1f), MoveAcceptableRadius=%.1f, TurnSpeed=%.1f"),
		*GetName(),
		static_cast<int32>(MonsterType),
		GetAIMaxHealth(),
		GetAIMoveSpeed(),
		GetAIAttackPower(),
		GetAIAttackRange(),
		GetAICollisionRadius(),
		GetCapsuleComponent()->GetScaledCapsuleRadius(),
		GetAIMoveAcceptableRadius(),
		GetAITurnSpeed());

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

	// [임시 디버그] player 다운 시 몬스터가 실제로 계속 틱되고 있는지 확인용 (1초에 한 번만 출력)
	DebugTickLogAccum += DeltaSeconds;
	if (DebugTickLogAccum >= 1.0f)
	{
		DebugTickLogAccum = 0.f;
		UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s Tick 살아있음 - CurrentCCState=%d"), *GetName(), static_cast<uint8>(CurrentCCState));
	}

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
	// 스윕 시작점을 액터 피벗(캡슐 중심)이 아니라 "자기 몸통 표면"에서 출발하도록 자신의
	// 콜리전 반경만큼 앞으로 밀어줌. 기존엔 피벗에서 AttackRange만큼만 재서, 일반/탱커처럼
	// 캡슐이 작은 몬스터는 티가 안 났지만 보스처럼 캡슐이 큰 몬스터는 실제 몸통 밖으로 뻗는
	// 유효 사거리가 그만큼 짧아져서 육안상 딱 붙어있어도 스윕이 플레이어까지 안 닿는 문제가 있었음
	const float SelfRadius = GetAICollisionRadius();
	const FVector SweepStart = GetActorLocation() + GetActorForwardVector() * SelfRadius;
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

	// [임시 디버그] 실제 스윕 범위/결과 확인용 - AnimNotify는 호출되는데 데미지가 안 들어가는 경우와
	// AnimNotify 자체가 안 불리는 경우를 구분하기 위함
	UE_LOG(LogTemp, Warning,
		TEXT("[임시 디버그] %s AttackHitCheck - SelfRadius=%.1f, AttackRange=%.1f, Start=%s, End=%s, bResult=%d, HitActor=%s"),
		*GetName(),
		SelfRadius,
		GetAIAttackRange(),
		*SweepStart.ToString(),
		*SweepEnd.ToString(),
		bResult ? 1 : 0,
		(bResult && HitResult.GetActor()) ? *HitResult.GetActor()->GetName() : TEXT("NULL"));

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
					// 몽타주가 끝난 뒤 잠깐이라도 대기하면, 그 사이에 애님 그래프가 베이스 포즈(Idle)로
					// 블렌드백되면서 몬스터가 다시 일어서는 것처럼 보이는 문제가 있어 지연 없이 바로 파괴함.
					// (애님 그래프에 "사망 상태 유지"용 스테이트를 추가하는 게 근본적인 해결책이라 추후 필요)
					Destroy();
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
		UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s PlayAttackMontage: %s 재생 시작"), *GetName(), *Montage->GetName());

		AddCCState(ECPMonsterCCState::Attacking);

		AnimInstance->StopAllMontages(0.0f);
		AnimInstance->Montage_Play(Montage, 1.0f);

		FOnMontageEnded MontageEndDelegate;
		MontageEndDelegate.BindUObject(this, &ACPMonsterBase::NotifyAttackActionEnd);

		AnimInstance->Montage_SetEndDelegate(MontageEndDelegate, Montage);
	}
	else
	{
		// 여기로 빠지면 몽타주가 재생되지 않고, BT의 Attack 태스크도 완료 델리게이트를 못 받아서 InProgress로 멈춰있게 됨
		UE_LOG(LogTemp, Error, TEXT("[임시 디버그] %s PlayAttackMontage 실패 - AnimInstance=%s, Montage=%s"),
			*GetName(),
			AnimInstance ? TEXT("Valid") : TEXT("NULL"),
			Montage ? *Montage->GetName() : TEXT("NULL"));
	}
}

float ACPMonsterBase::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 포효 등으로 무적 상태면 데미지 무시
	if (HasCCState(ECPMonsterCCState::Invulnerable))
	{
		// [임시 디버그] 무적 상태에서 들어온 데미지가 실제로 무시되는지 확인용
		UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s TakeDamage 무시됨(무적) - DamageAmount=%.1f, CurrentCCState=%d, CurrentHealth=%.1f"),
			*GetName(), DamageAmount, static_cast<uint8>(CurrentCCState), StatComponent ? StatComponent->CurrentHealth : -1.f);
		return 0.f;
	}

	if (StatComponent)
	{
		const float HealthBefore = StatComponent->CurrentHealth;
		StatComponent->CurrentHealth -= DamageAmount;

		// [임시 디버그] 무적이 아닐 때 실제로 얼마나 깎이는지, bIsDead/bPendingDeath 상태 확인용
		UE_LOG(LogTemp, Warning,
			TEXT("[임시 디버그] %s TakeDamage 적용됨 - DamageAmount=%.1f, HealthBefore=%.1f, HealthAfter=%.1f, bIsDead=%d, bPendingDeath=%d"),
			*GetName(), DamageAmount, HealthBefore, StatComponent->CurrentHealth, bIsDead, bPendingDeath);

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

	// KnockbackDuration 안에 넉백이 연속으로 들어오면 이전 복구 타이머가 지금 넉백을 중간에 취소시켜버리므로,
	// 새로 걸기 전에 이전 타이머부터 취소함
	GetWorldTimerManager().ClearTimer(KnockbackRestoreHandle);

	// KnockbackDuration 후 RVO 회피를 복구하고 Flying 복구
	TWeakObjectPtr<ACPMonsterBase> WeakThis(this);
	GetWorldTimerManager().SetTimer(KnockbackRestoreHandle, [WeakThis, bWasFlying, PausedMoveRequestID]()
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
	UE_LOG(LogTemp, Warning, TEXT("[임시 디버그] %s NotifyAttackActionEnd 호출됨 - bInterrupted=%d"), *GetName(), bInterrupted);

	RemoveCCState(ECPMonsterCCState::Attacking);

	OnAttackFinished.ExecuteIfBound();
}

void ACPMonsterBase::CancelAIAttack()
{
	// BT의 Attack 태스크가 Abort된 경우 호출됨(예: 타겟이 사라져서 상위 데코레이터가 강제 중단시킬 때).
	// 이 경우 몽타주가 자연 종료(NotifyAttackActionEnd)될 기회를 못 얻으므로, 여기서 직접 몽타주를 멈추고
	// Attacking CC 상태를 해제해줘야 애니메이션이 공격 포즈에 멈춰있지 않고 Idle로 돌아감
	if (!HasCCState(ECPMonsterCCState::Attacking))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.1f);
	}

	RemoveCCState(ECPMonsterCCState::Attacking);
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
float ACPMonsterBase::GetAIAttackInterval()
{
	return StatComponent ? StatComponent->DefaultStat.AttackInterval : 1.0f;
}

float ACPMonsterBase::GetAICollisionRadius()
{
	return StatComponent ? StatComponent->DefaultStat.CollisionRadius : 0.0f;
}

float ACPMonsterBase::GetAICollisionHalfHeight()
{
	return StatComponent ? StatComponent->DefaultStat.CollisionHalfHeight : 0.0f;
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

float ACPMonsterBase::GetSpawnHeightOffset() const
{
	return GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
}
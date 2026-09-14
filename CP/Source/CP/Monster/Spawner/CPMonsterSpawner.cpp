// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/Spawner/CPMonsterSpawner.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Monster/CPMonsterBase.h"
#include "Monster/Pool/CPMonsterPoolSubsystem.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AI/Navigation/NavAgentInterface.h"

// Sets default values
ACPMonsterSpawner::ACPMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

bool ACPMonsterSpawner::ResolveFreeSpawnLocation(const FVector& InDesiredLocation, const FNavAgentProperties& InNavAgentProps, const FVector& InPlayerLocation, FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapCheckRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const float MinPlayerDistSq = FMath::Square(MinPlayerSpawnDistance);

	// 겹침 / 플레이어 근접 / NavMesh 고립 섬(끼임) 여부를 한데 모아 판정하는 공통 후보 검증
	auto TryResolveAt = [&](const FVector& Candidate, FVector& OutResolved)
	{
		if (MinPlayerSpawnDistance > 0.f && FVector::DistSquared(Candidate, InPlayerLocation) < MinPlayerDistSq)
		{
			return false;
		}

		if (World->OverlapAnyTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, ProbeShape, QueryParams))
		{
			return false;
		}

		return ProjectToNavMesh(Candidate, InNavAgentProps, OutResolved) && IsLocationReachableFromArenaCenter(OutResolved, InNavAgentProps);
	};

	// 원래 위치부터 시도
	if (TryResolveAt(InDesiredLocation, OutLocation))
	{
		return true;
	}

	// 막혔거나(겹침/플레이어 근접) 원래 위치가 NavMesh 밖/고립 섬이면, 주변을 원형으로 훑어서 유효한 자리를 찾음
	for (int32 Attempt = 1; Attempt <= MaxRelocationAttempts; ++Attempt)
	{
		const float AngleDeg = (360.f / MaxRelocationAttempts) * Attempt;
		const FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(AngleDeg)), FMath::Sin(FMath::DegreesToRadians(AngleDeg)), 0.f) * RelocationStepDistance;
		const FVector Candidate = InDesiredLocation + Offset;

		if (TryResolveAt(Candidate, OutLocation))
		{
			return true;
		}
	}

	// 마지막 수단: 겹침/플레이어 근접 여부와 무관하게 원래 위치라도 NavMesh에 투영되면 사용(겹침은
	// SpawnActor의 AdjustIfPossibleButAlwaysSpawn이 물리적으로 밀어내 줌). 단, 고립 섬 여부는 여기서도
	// 반드시 걸러야 함 - 이 체크가 없으면 일반 몬스터도 원형 재탐색을 다 돌고도 결국 여기서 고립된 자리를
	// "찾음"으로 잘못 반환해 SpawnMonsterRow의 continue(스킵)를 못 타고 그 자리에 갇혀버림(끼임).
	// 이것마저 실패하면(NavMesh 자체가 없거나 투영 범위 밖, 또는 고립 섬) 더 이상 보정할 방법이 없으므로
	// 이 자리는 스폰 불가로 판단해서 호출부에 알림
	return ProjectToNavMesh(InDesiredLocation, InNavAgentProps, OutLocation) && IsLocationReachableFromArenaCenter(OutLocation, InNavAgentProps);
}

bool ACPMonsterSpawner::IsLocationReachableFromArenaCenter(const FVector& InLocation, const FNavAgentProperties& InNavAgentProps) const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavSys)
	{
		return true;
	}

	const UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(World, InLocation, ArenaCenterLocation);

	// 부분 경로(Partial)는 중간에 끊겨 중심까지 못 이어진 것 - 고립된 NavMesh 조각에 스폰된 경우를 걸러냄
	return Path && Path->IsValid() && !Path->IsPartial();
}

bool ACPMonsterSpawner::ProjectToNavMesh(const FVector& InLocation, const FNavAgentProperties& InNavAgentProps, FVector& OutLocation, float InExtentXY, float InExtentZ) const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavSys)
	{
		return false;
	}

	FNavLocation OutNavLocation;
	if (NavSys->ProjectPointToNavigation(InLocation, OutNavLocation, FVector(InExtentXY, InExtentXY, InExtentZ), &InNavAgentProps))
	{
		OutLocation = OutNavLocation.Location;
		return true;
	}

	// 투영 범위 안에 NavMesh가 전혀 없음 - 검증 안 된 위치를 검증된 것처럼 돌려주지 않고 실패로 알림
	return false;
}

TArray<ACPMonsterBase*> ACPMonsterSpawner::SpawnMonsterRow(TSubclassOf<ACPMonsterBase> MonsterClass, ECPMonsterType InMonsterType, int32 InCount, float InRowSpacingY, int32 InRound, int32 InWave, const FVector& InPlayerLocation, bool bAllowFallbackOutsideNavMesh)
{
	TArray<ACPMonsterBase*> SpawnedMonsters;

	if (!IsValid(MonsterClass) || InCount <= 0 || !GetWorld())
	{
		return SpawnedMonsters;
	}

	FTransform BaseTransform = SpawnCapsule->GetComponentTransform();
	const FVector RightAxis = FVector::YAxisVector; // 스포너 회전과 무관하게 항상 월드 Y축 기준으로 나란히 배치

	// 줄 간격이 몬스터 Radius보다 좁으면 스폰 직후 서로 겹쳐서 밀려남 - 최소 간격을 강제 보장
	float EffectiveRowSpacingY = InRowSpacingY;

	// NavMesh 투영 시 몬스터 크기에 맞는 Supported Agent를 고르기 위한 값(Default 0/0이면 기본 Agent로 투영)
	FNavAgentProperties SpawnNavAgentProps;

	// true면(Ranged/Bomb 등 비행형) 아래 ResolveFreeSpawnLocation의 NavMesh 투영 결과 Z를 무시하고
	// 이 스폰 높이를 그대로 강제함 - NavMesh 투영은 항상 "NavMesh 표면(지면) 위의 점"을 돌려주므로,
	// 그대로 두면 고정 비행 고도가 지면 높이로 끌려 내려가 파묻혀버림
	bool bUseFixedSpawnHeight = false;

	// 몬스터 클래스마다 캡슐 Half Height(또는 비행 몬스터의 고정 스폰 높이)가 달라서, SpawnCapsule의
	// 고정 Z(90)를 그대로 쓰면 살짝 떠서 스폰됐다가 떨어지거나 파묻히는 문제가 생김.
	// 스포너 액터 자체가 지면에 놓여있다고 가정하고, 그 위로 몬스터별 스폰 높이만큼만 띄움
	float SpawnHeightOffset = 0.f;

	if (ACPMonsterBase* MonsterCDO = MonsterClass->GetDefaultObject<ACPMonsterBase>())
	{
		SpawnHeightOffset = MonsterCDO->GetSpawnHeightOffset();

		FVector SpawnLocation = BaseTransform.GetLocation();
		SpawnLocation.Z = GetActorLocation().Z + SpawnHeightOffset;
		BaseTransform.SetLocation(SpawnLocation);

		EffectiveRowSpacingY = FMath::Max(InRowSpacingY, MonsterCDO->GetAICollisionRadius() * 2.f);

		SpawnNavAgentProps.AgentRadius = MonsterCDO->GetAICollisionRadius();
		SpawnNavAgentProps.AgentHeight = MonsterCDO->GetAICollisionHalfHeight() * 2.f;

		bUseFixedSpawnHeight = MonsterCDO->ShouldUseFixedSpawnHeight();
	}

	UCPMonsterPoolSubsystem* Pool = GetWorld()->GetSubsystem<UCPMonsterPoolSubsystem>();

	SpawnedMonsters.Reserve(InCount);

	for (int32 i = 0; i < InCount; ++i)
	{
		// (현재 인덱스 - 가운데 인덱스) 떨어진 만큼 옆으로
		const float Offset = (i - (InCount - 1) / 2.0f) * EffectiveRowSpacingY;

		FTransform SpawnTransform = BaseTransform;
		SpawnTransform.AddToTranslation(RightAxis * Offset);

		const float DesiredSpawnZ = SpawnTransform.GetLocation().Z;

		// 이 자리에 이미 다른 몬스터/장애물이 있으면 주변의 비어있는 자리로 대신 스폰함
		FVector ResolvedLocation;
		const bool bResolved = ResolveFreeSpawnLocation(SpawnTransform.GetLocation(), SpawnNavAgentProps, InPlayerLocation, ResolvedLocation);

		if (!bResolved)
		{
			if (!bAllowFallbackOutsideNavMesh)
			{
				// 검증 안 된(NavMesh 밖) 위치에는 스폰하지 않고 이 마리만 건너뜀 - AI가 NavMesh 밖에서
				// 먹통이 되는 문제를 근본적으로 막기 위함(원래 위치를 검증된 것처럼 속이지 않음)
				UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawner] SpawnMonsterRow - NavMesh 투영 실패로 스폰을 건너뜁니다 (위치 %s)."), *SpawnTransform.GetLocation().ToString());
				continue;
			}

			// 보스 등 절대 스폰이 스킵되면 안 되는 경우 - 2단계 Resilient 전략:
			// 1) 기본 범위(200)에서 실패했으니, 훨씬 넓은 범위로 한 번 더 NavMesh를 찾아봄
			constexpr float WideProjectionExtentXY = 5000.f;
			constexpr float WideProjectionExtentZ = 2000.f;
			if (ProjectToNavMesh(SpawnTransform.GetLocation(), SpawnNavAgentProps, ResolvedLocation, WideProjectionExtentXY, WideProjectionExtentZ)
				&& IsLocationReachableFromArenaCenter(ResolvedLocation, SpawnNavAgentProps))
			{
				UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawner] SpawnMonsterRow - 기본 범위 NavMesh 투영 실패, 넓은 범위 재탐색으로 보정했습니다 (위치 %s)."), *SpawnTransform.GetLocation().ToString());
			}
			else
			{
				// 2) 그마저도 실패하면 - 오너(플레이어)는 항상 NavMesh 위에 서 있어야 이동이 가능하므로,
				// 오너 위치를 기준으로 다시 넓은 범위 탐색함. "보스는 무조건 스폰"이 "무조건 NavMesh 밖에라도
				// 스폰"으로 새는 걸 막기 위한 안전망 - 이 스포너 주변 NavMesh가 끊겨있어도 오너 근방엔
				// 거의 항상 NavMesh가 있음
				AActor* AttachOwner = GetAttachParentActor();
				const FVector OwnerLocation = AttachOwner ? AttachOwner->GetActorLocation() : GetActorLocation();
				if (ProjectToNavMesh(OwnerLocation, SpawnNavAgentProps, ResolvedLocation, WideProjectionExtentXY, WideProjectionExtentZ)
					&& IsLocationReachableFromArenaCenter(ResolvedLocation, SpawnNavAgentProps))
				{
					UE_LOG(LogTemp, Warning, TEXT("[CPMonsterSpawner] SpawnMonsterRow - 넓은 범위 재탐색도 실패, 오너 위치 기준으로 보정했습니다 (오너 위치 %s)."), *OwnerLocation.ToString());
				}
				else
				{
					// 3) 그마저도 실패하면(NavMesh 시스템 자체가 없거나 레벨에 전혀 안 빌드됨) 최후의 수단으로
					// 원래 위치에 강제 스폰 - 게임 진행이 막히는 것보다는 낫지만, 레벨의 NavMesh 커버리지가
					// 심각하게 잘못됐다는 뜻이므로 Error로 남김
					UE_LOG(LogTemp, Error, TEXT("[CPMonsterSpawner] SpawnMonsterRow - CRITICAL: 오너 위치 기준 재탐색도 실패해 NavMesh 밖에 강제 스폰합니다 (위치 %s)."), *SpawnTransform.GetLocation().ToString());
					ResolvedLocation = SpawnTransform.GetLocation();
				}
			}
		}

		if (bUseFixedSpawnHeight)
		{
			// NavMesh 투영이 돌려준 XY(장애물 회피/유효 위치)는 그대로 쓰되, Z만 원래 의도한
			// 고정 비행 고도로 복원 - 안 그러면 투영 결과의 지면 높이로 도로 끌려 내려감
			ResolvedLocation.Z = DesiredSpawnZ;
		}
		else
		{
			// NavMesh가 돌려주는 Z는 지면 근사치(리캐스트 생성 시 셀 높이 등으로 실제 충돌 지면과
			// 몇 cm 오차가 남을 수 있음)라, 그대로 쓰면 캡슐이 큰 몬스터(보스 등)일수록 그 오차가
			// "살짝 뜬 채 스폰됐다가 떨어짐"으로 눈에 띔. NavMesh 결과는 XY(장애물 회피/유효 위치
			// 판정)만 신뢰하고, Z는 그 XY 지점에서 실제 지면을 다시 라인트레이스해서 정확히 맞춤
			constexpr float GroundTraceUp = 500.f;
			constexpr float GroundTraceDown = 2000.f;
			const FVector TraceStart(ResolvedLocation.X, ResolvedLocation.Y, ResolvedLocation.Z + GroundTraceUp);
			const FVector TraceEnd(ResolvedLocation.X, ResolvedLocation.Y, ResolvedLocation.Z - GroundTraceDown);

			FHitResult GroundHit;
			FCollisionQueryParams TraceParams(NAME_None, false, this);
			if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
			{
				ResolvedLocation.Z = GroundHit.Location.Z + SpawnHeightOffset;
			}
		}
		SpawnTransform.SetLocation(ResolvedLocation);

		ACPMonsterBase* SpawnedMonster = Pool ? Pool->Acquire(InMonsterType, MonsterClass, SpawnTransform) : nullptr;
		if (SpawnedMonster)
		{
			SpawnedMonster->ApplyWaveStat(InRound, InWave);
			SpawnedMonsters.Add(SpawnedMonster);
		}
	}

	return SpawnedMonsters;
}

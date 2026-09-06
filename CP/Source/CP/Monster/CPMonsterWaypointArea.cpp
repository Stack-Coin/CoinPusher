// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/CPMonsterWaypointArea.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "Engine/OverlapResult.h"

ACPMonsterWaypointArea::ACPMonsterWaypointArea()
{
	PrimaryActorTick.bCanEverTick = false;

	AreaShape = CreateDefaultSubobject<USphereComponent>(TEXT("AreaShape"));
	RootComponent = AreaShape;

	AreaShape->InitSphereRadius(1000.f);

	AreaShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaShape->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaShape->SetGenerateOverlapEvents(false);
	AreaShape->ShapeColor = FColor::Cyan;
}

bool ACPMonsterWaypointArea::GetRandomPointInArea(FVector& OutPoint, const AActor* RequestingActor) const
{
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	if (!NavSystem || !AreaShape)
	{
		return false;
	}

	const FVector Center = GetActorLocation();
	const float Radius = AreaShape->GetScaledSphereRadius();

	// 이 반경 안에 이미 다른 몬스터가 있으면 붐비는 지점으로 판단 // 후보에서 제외함
	constexpr float MinDistanceFromOtherMonsters = 200.f;
	constexpr int32 MaxAttempts = 10;

	FVector BestCandidate = Center;
	bool bFoundValidOnMesh = false;

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		// UE 기본 GetRandomPointInNavigableRadius는 NavMesh가 원의 일부만 덮고 있으면(예: 원이 벽/장애물에
		// 걸쳐 있는 경우) 결과가 그 걸리기 쉬운 한쪽으로 쏠리는 경향이 있음 - 이게 몬스터들이 전부 거의
		// 같은 지점에 모이는 원인 중 하나. 그래서 여기선 원 전체 "면적"에 고르게 퍼지도록 직접 각도/반경을
		// 뽑아 후보 지점을 만든 다음 NavMesh 위로 투영함.
		// (반경을 그냥 RandRange(0,R)로 뽑으면 중심 쪽으로 쏠리기 때문에 sqrt(rand)로 보정해서 균등 분포시킴)
		const float Angle = FMath::FRand() * 2.f * UE_PI;
		const float Distance = FMath::Sqrt(FMath::FRand()) * Radius;
		const FVector Candidate = Center + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);

		FNavLocation NavLocation;
		if (!NavSystem->ProjectPointToNavigation(Candidate, NavLocation, FVector(200.f, 200.f, 500.f)))
		{
			continue;
		}

		if (!bFoundValidOnMesh)
		{
			// NavMesh 위에 있는 후보를 하나라도 찾으면 최소한의 폴백으로 저장해둠
			BestCandidate = NavLocation.Location;
			bFoundValidOnMesh = true;
		}

		// 이 지점 근처에 이미 다른 몬스터가 몰려있으면 이 후보는 건너뛰고 다시 뽑음
		// (같은 웨이포인트 영역을 여러 몬스터가 동시에 노릴 때 한 지점에 뭉치는 병목을 줄이기 위함)
		if (RequestingActor && GetWorld())
		{
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams Params(NAME_None, false, RequestingActor);
			const bool bCrowded = GetWorld()->OverlapMultiByObjectType
			(
				Overlaps,
				NavLocation.Location,
				FQuat::Identity,
				FCollisionObjectQueryParams(ECC_Pawn),
				FCollisionShape::MakeSphere(MinDistanceFromOtherMonsters),
				Params
			);

			if (bCrowded)
			{
				continue;
			}
		}

		OutPoint = NavLocation.Location;
		return true;
	}

	// 몇 번을 시도해도 안 붐비는 지점을 못 찾았으면, NavMesh 위에서 찾았던 지점 중 아무거나 사용
	if (bFoundValidOnMesh)
	{
		OutPoint = BestCandidate;
		return true;
	}

	// 기존 방식으로 한 번 더 시도
	FNavLocation FallbackLocation;
	if (NavSystem->GetRandomPointInNavigableRadius(Center, Radius, FallbackLocation))
	{
		OutPoint = FallbackLocation.Location;
		return true;
	}

	return false;
}

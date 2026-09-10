// Fill out your copyright notice in the Description page of Project Settings.


#include "CPCoinTowerSpawner.h"
#include "CPCoin.h"
#include "CPPusher.h"
#include "Log/CPLogCategories.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ACPCoinTowerSpawner::ACPCoinTowerSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = SpawnerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnerRoot"));

	TowerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TowerRoot"));
	TowerRoot->SetupAttachment(SpawnerRoot);
}

void ACPCoinTowerSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMovingPusherBack && TargetPusher)
	{
		PusherMoveElapsedTime += DeltaTime;

		// Pusher를 현재 위치에서 BackMoveTime 동안 BackPosition까지 선형 보간으로 이동
		const float MoveAlpha = (BackMoveTime > 0.0f) ? FMath::Clamp(PusherMoveElapsedTime / BackMoveTime, 0.0f, 1.0f) : 1.0f;
		TargetPusher->SetActorLocation(FMath::Lerp(PusherMoveStartLocation, PusherMoveTargetLocation, MoveAlpha));

		if (MoveAlpha >= 1.0f)
		{
			bIsMovingPusherBack = false;
		}
	}

	if (bIsReturningPusher && TargetPusher)
	{
		PusherReturnElapsedTime += DeltaTime;

		// Pusher를 BackPosition에서 PusherReturnTime 동안 원래 위치(PusherMoveStartLocation)까지 선형 보간으로 복귀
		const float ReturnAlpha = (PusherReturnTime > 0.0f) ? FMath::Clamp(PusherReturnElapsedTime / PusherReturnTime, 0.0f, 1.0f) : 1.0f;
		TargetPusher->SetActorLocation(FMath::Lerp(PusherReturnStartLocation, PusherMoveStartLocation, ReturnAlpha));

		if (ReturnAlpha >= 1.0f)
		{
			bIsReturningPusher = false;

			// 원래 위치까지 다 돌아왔으므로 왕복 운동 재개
			TargetPusher->SetPusherPaused(false);

			// 8. 코인 Detach에 이어 Pusher 복귀+재개까지 모두 끝났으므로 다음 SpawnTower() 호출을 다시 허용
			bIsTowerActive = false;
		}
	}

	if (!bIsRising)
	{
		return;
	}

	RiseElapsedTime += DeltaTime;

	// 3. 스폰 지점(상대 위치 0)에서 CoinTowerPosition까지 UpTime 동안 선형 보간으로 상승
	const float Alpha = (UpTime > 0.0f) ? FMath::Clamp(RiseElapsedTime / UpTime, 0.0f, 1.0f) : 1.0f;
	TowerRoot->SetRelativeLocation(FMath::Lerp(FVector::ZeroVector, CoinTowerPosition, Alpha));

	if (Alpha >= 1.0f)
	{
		bIsRising = false;
		CompleteRise();
	}
}

void ACPCoinTowerSpawner::SpawnTower(FName ItemID, int32 N)
{
	// 8. 이전 타워가 완전히 끝나기(코인 Detach + Pusher 재개) 전까지는 새로 스폰할 수 없음
	if (bIsTowerActive || N <= 0 || !CoinClass)
	{
		return;
	}

	bIsTowerActive = true;

	// 이번 타워를 스폰 지점(상대 위치 0)부터 다시 시작
	RiseElapsedTime = 0.0f;
	bIsRising = false;
	TowerRoot->SetRelativeLocation(FVector::ZeroVector);

	// 2. 스폰이 이루어지는 동안 Pusher를 멈추고, BackMoveTime 동안 현재 위치에서 BackPosition(Pusher 기준
	// 상대 위치)까지 이동시키기 시작 (실제 이동은 Tick에서 진행)
	if (TargetPusher)
	{
		TargetPusher->SetPusherPaused(true);

		PusherMoveElapsedTime = 0.0f;
		PusherMoveStartLocation = TargetPusher->GetActorLocation();
		PusherMoveTargetLocation = TargetPusher->GetActorTransform().TransformPosition(BackPosition);
		bIsMovingPusherBack = true;
	}

	// 1. N개 층 * CoinsPerFloor(5)개를 원형으로 SpawnActor 스폰
	SpawnTowerCoins(N);

	// 3. 스폰이 끝났으므로 상승 시작 (다음 Tick부터 진행)
	bIsRising = true;
}

void ACPCoinTowerSpawner::SpawnTowerCoins(int32 FloorCount)
{
	UE_LOG(LogCoinPusher, Warning, TEXT("Spawning"));
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TowerCoins.Reset();
	TowerCoins.Reserve(FloorCount * CoinsPerFloor);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const float SlotAngleStep = 2.0f * PI / CoinsPerFloor;

	for (int32 Floor = 0; Floor < FloorCount; ++Floor)
	{
		// 한 층씩 걸러 반 칸(SlotAngleStep의 절반)씩 회전시켜, 바로 아랫층 코인들의 틈 사이사이에
		// 윗층 코인이 놓이도록 함 (벽돌쌓기처럼 층마다 어긋나게 배치)
		const float FloorAngleOffset = (Floor % 2 == 1) ? (SlotAngleStep * 0.5f) : 0.0f;

		for (int32 Slot = 0; Slot < CoinsPerFloor; ++Slot)
		{
			const float Angle = SlotAngleStep * Slot + FloorAngleOffset;
			const FVector LocalOffset(TowerRadius * FMath::Cos(Angle), TowerRadius * FMath::Sin(Angle), Floor * FloorHeight);
			const FVector SpawnLocation = TowerRoot->GetComponentTransform().TransformPosition(LocalOffset);

			if (ACPCoin* SpawnedCoin = World->SpawnActor<ACPCoin>(CoinClass, SpawnLocation, GetActorRotation(), SpawnParams))
			{
				// 4. 상승하는 동안 중력/물리충돌(다른 코인 제외)/Launch/타입 변경을 모두 잠금
				SpawnedCoin->SetTowerLocked(true);

				// TowerRoot에 부착해 상승 애니메이션에 함께 딸려오게 함
				SpawnedCoin->AttachToComponent(TowerRoot, FAttachmentTransformRules::KeepWorldTransform);

				TowerCoins.Add(SpawnedCoin);
			}
		}
	}
}

void ACPCoinTowerSpawner::CompleteRise()
{
	// 5. + 6. 잠금 해제(중력/물리충돌/Launch/타입 변경 복구) + 독립 액터로 Detach
	for (const TObjectPtr<ACPCoin>& TowerCoin : TowerCoins)
	{
		if (!TowerCoin)
		{
			continue;
		}

		TowerCoin->SetTowerLocked(false);
		TowerCoin->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	TowerCoins.Reset();

	// 7. Pusher를 BackPosition에서 원래 위치까지 PusherReturnTime 동안 천천히 되돌리는 애니메이션을 시작.
	// 왕복 운동 재개(SetPusherPaused(false))와 bIsTowerActive 해제는 그 애니메이션이 끝났을 때 Tick에서 처리
	if (TargetPusher)
	{
		// 후퇴 애니메이션이 아직 끝나지 않았다면 즉시 완료시켜 정확히 BackPosition에서 복귀를 시작하도록 보장
		if (bIsMovingPusherBack)
		{
			TargetPusher->SetActorLocation(PusherMoveTargetLocation);
			bIsMovingPusherBack = false;
		}

		PusherReturnElapsedTime = 0.0f;
		PusherReturnStartLocation = TargetPusher->GetActorLocation();
		bIsReturningPusher = true;
	}
	else
	{
		// 연결된 Pusher가 없으면 기다릴 대상이 없으므로 바로 다음 SpawnTower() 호출을 허용
		bIsTowerActive = false;
	}
}

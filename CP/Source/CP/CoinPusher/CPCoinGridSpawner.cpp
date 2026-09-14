// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPCoinGridSpawner.h"
#include "CPCoin.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Log/CPLogCategories.h"

ACPCoinGridSpawner::ACPCoinGridSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnVolume"));
	SpawnVolume->SetBoxExtent(FVector(200.0f, 200.0f, 50.0f));
	SpawnVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACPCoinGridSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void ACPCoinGridSpawner::SpawnCoins()
{
	if (!CoinClass || CoinCount <= 0 || !GetWorld())
	{
		return;
	}

	// N개를 담을 수 있는, 가능한 한 정사각형에 가까운 격자(Columns x Rows)를 계산
	const int32 Columns = FMath::Max(FMath::CeilToInt(FMath::Sqrt(static_cast<float>(CoinCount))), 1);
	const int32 Rows = FMath::Max(FMath::CeilToInt(static_cast<float>(CoinCount) / Columns), 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 Index = 0; Index < CoinCount; ++Index)
	{
		const FVector LocalLocation = CalculateGridJitterLocalLocation(Index, Columns, Rows);
		const FVector SpawnLocation = GetActorTransform().TransformPosition(LocalLocation);

		GetWorld()->SpawnActor<ACPCoin>(CoinClass, SpawnLocation, GetActorRotation(), SpawnParams);
	}

	UE_LOG(LogCoinPusher, Log, TEXT("[ACPCoinGridSpawner] Spawned %d coin(s) in a %dx%d grid within SpawnVolume."),
		CoinCount, Columns, Rows);
}

FVector ACPCoinGridSpawner::CalculateGridJitterLocalLocation(int32 Index, int32 Columns, int32 Rows) const
{
	const FVector BoxExtent = SpawnVolume->GetUnscaledBoxExtent();

	const int32 Row = Index / Columns;
	const int32 Column = Index % Columns;

	const float CellSizeX = (BoxExtent.X * 2.0f) / Columns;
	const float CellSizeY = (BoxExtent.Y * 2.0f) / Rows;

	// 각 셀의 중심 좌표 (Box 로컬 원점 기준, -X/-Y 모서리부터 채워나감)
	float LocalX = -BoxExtent.X + CellSizeX * (Column + 0.5f);
	float LocalY = -BoxExtent.Y + CellSizeY * (Row + 0.5f);

	// Jitter: 셀 크기의 JitterRatio 비율만큼 무작위로 흔들어 완전한 격자로 보이지 않게 하고,
	// 코인끼리 정확히 겹쳐 스폰되는 것을 방지한다
	LocalX += FMath::FRandRange(-CellSizeX * 0.5f, CellSizeX * 0.5f) * JitterRatio;
	LocalY += FMath::FRandRange(-CellSizeY * 0.5f, CellSizeY * 0.5f) * JitterRatio;

	const float LocalZ = bJitterHeight ? FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z) : 0.0f;

	return FVector(LocalX, LocalY, LocalZ);
}

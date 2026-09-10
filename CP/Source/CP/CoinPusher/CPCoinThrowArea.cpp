// Fill out your copyright notice in the Description page of Project Settings.


#include "CPCoinThrowArea.h"
#include "CPCoin.h"
#include "Components/BoxComponent.h"

ACPCoinThrowArea::ACPCoinThrowArea()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ThrowVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ThrowVolume"));

	ThrowVolume->SetBoxExtent(FVector(150.0f, 150.0f, 150.0f));
	// Overlap만 감지하고 아무것도 물리적으로 막지 않음
	ThrowVolume->SetCollisionProfileName(FName("OverlapAllDynamic"));
}

void ACPCoinThrowArea::ActiveThrow()
{
	if (!ThrowVolume)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	ThrowVolume->GetOverlappingActors(OverlappingActors, ACPCoin::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		ACPCoin* Coin = Cast<ACPCoin>(OverlappingActor);
		if (!Coin)
		{
			continue;
		}

		const float UpPower = bIsRandomize ? FMath::FRandRange(MinUpPower, MaxUpPower) : MaxUpPower;
		const float ForwardPower = bIsRandomize ? FMath::FRandRange(MinForwardPower, MaxForwardPower) : MaxForwardPower;

		// 위/앞 방향은 이 액터(또는 코인)의 회전과 무관하게 항상 월드 Z/X축을 기준으로 함
		Coin->Launch(FVector(ForwardPower, 0.0f, UpPower));
	}
}

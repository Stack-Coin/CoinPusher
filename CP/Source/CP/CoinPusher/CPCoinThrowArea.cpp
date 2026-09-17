// Fill out your copyright notice in the Description page of Project Settings.


#include "CPCoinThrowArea.h"
#include "CPCoin.h"
#include "CPItem.h"
#include "Components/BoxComponent.h"

ACPCoinThrowArea::ACPCoinThrowArea()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ThrowVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ThrowVolume"));

	ThrowVolume->SetBoxExtent(FVector(150.0f, 150.0f, 150.0f));
	// Overlap만 감지하고 아무것도 물리적으로 막지 않음
	ThrowVolume->SetCollisionProfileName(FName("OverlapAllDynamic"));
}

void ACPCoinThrowArea::ActiveThrow(AActor* ActorToExclude)
{
	if (!ThrowVolume)
	{
		return;
	}

	// Coin뿐 아니라 Item도 동일하게 날려보내야 하므로 클래스 필터 없이 겹친 모든 액터를 가져와
	// Coin/Item 각각으로 캐스트해본다
	TArray<AActor*> OverlappingActors;
	ThrowVolume->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor == ActorToExclude)
		{
			continue;
		}

		ACPCoin* Coin = Cast<ACPCoin>(OverlappingActor);
		ACPItem* Item = Coin ? nullptr : Cast<ACPItem>(OverlappingActor);
		if (!Coin && !Item)
		{
			continue;
		}

		const float UpPower = bIsRandomize ? FMath::FRandRange(MinUpPower, MaxUpPower) : MaxUpPower;
		const float ForwardPower = bIsRandomize ? FMath::FRandRange(MinForwardPower, MaxForwardPower) : MaxForwardPower;
		const FVector LaunchVelocity(ForwardPower, 0.0f, UpPower);

		// 위/앞 방향은 이 액터(또는 코인/아이템)의 회전과 무관하게 항상 월드 Z/X축을 기준으로 함
		if (Coin)
		{
			Coin->Launch(LaunchVelocity);
		}
		else
		{
			Item->Launch(LaunchVelocity);
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CPPassiveCoinConvertArea.h"
#include "CPCoin.h"
#include "Components/BoxComponent.h"

ACPPassiveCoinConvertArea::ACPPassiveCoinConvertArea()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ConvertVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ConvertVolume"));

	ConvertVolume->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	// Overlap만 감지하고 아무것도 물리적으로 막지 않음
	ConvertVolume->SetCollisionProfileName(FName("OverlapAllDynamic"));
}

void ACPPassiveCoinConvertArea::ConvertActive(int32 Num)
{
	if (Num <= 0)
	{
		return;
	}

	//이미 Passive인 코인은 다시 골라봐야 아무 변화가 없으므로(SetCoinType이 조기 반환) 후보에서 제외
	TArray<ACPCoin*> Candidates = GatherCandidates([](const ACPCoin& Coin)
	{
			return (Coin.GetCoinType() == ECPCoinType::Normal);
	});

	ConvertRandomCandidates(MoveTemp(Candidates), Num, ECPCoinType::Passive);
}

void ACPPassiveCoinConvertArea::HPConvertActive(int32 Num)
{
	if (Num <= 0)
	{
		return;
	}

	//Normal 코인만 HP로 전환 대상이 됨
	TArray<ACPCoin*> Candidates = GatherCandidates([](const ACPCoin& Coin)
	{
		return (Coin.GetCoinType() == ECPCoinType::Normal);
	});

	ConvertRandomCandidates(MoveTemp(Candidates), Num, ECPCoinType::HP);
}

void ACPPassiveCoinConvertArea::MonsterConvertActive(int32 Num)
{
	if (Num <= 0)
	{
		return;
	}

	//Normal 코인만 Monster로 전환 대상이 됨
	TArray<ACPCoin*> Candidates = GatherCandidates([](const ACPCoin& Coin)
	{
		return (Coin.GetCoinType() == ECPCoinType::Normal);
	});

	ConvertRandomCandidates(MoveTemp(Candidates), Num, ECPCoinType::Monster);
}

TArray<ACPCoin*> ACPPassiveCoinConvertArea::GatherCandidates(TFunctionRef<bool(const ACPCoin&)> Predicate) const
{
	TArray<ACPCoin*> Candidates;

	if (!ConvertVolume)
	{
		return Candidates;
	}

	TArray<AActor*> OverlappingActors;
	ConvertVolume->GetOverlappingActors(OverlappingActors, ACPCoin::StaticClass());

	Candidates.Reserve(OverlappingActors.Num());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (ACPCoin* Coin = Cast<ACPCoin>(OverlappingActor))
		{
			if (Predicate(*Coin))
			{
				Candidates.Add(Coin);
			}
		}
	}

	return Candidates;
}

void ACPPassiveCoinConvertArea::ConvertRandomCandidates(TArray<ACPCoin*> Candidates, int32 Num, ECPCoinType TargetType)
{
	const int32 ConvertCount = FMath::Min(Num, Candidates.Num());
	for (int32 Index = 0; Index < ConvertCount; ++Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, Candidates.Num() - 1);
		Candidates[RandomIndex]->SetCoinType(TargetType);
		Candidates.RemoveAtSwap(RandomIndex);
	}
}

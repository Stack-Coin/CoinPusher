// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPRoulette.h"
#include "CPRouletteWidget.h"
#include "CPRouletteRewardReceiver.h"
#include "../CoinPusher/CPCoinPusher.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Player/CPGameMode.h"
#include "UObject/Class.h"

ACPRoulette::ACPRoulette()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
}

bool ACPRoulette::Roll()
{
	// 이미 스핀 중이면(다른 플레이어가 먼저 돌린 경우 포함) 무시 - 하나의 룰렛을 두 플레이어가 공유
	if (bIsRolling || Slots.Num() == 0)
	{
		return false;
	}


	bIsRolling = true;

	const int32 ResultIndex = PickWeightedSlotIndex();

	const TArray<UCPRouletteWidget*> Widgets = GetOrCreateRouletteWidgets();
	if (Widgets.Num() > 0)
	{
		// 로컬 스플릿 스크린의 모든 플레이어 화면에 동일한 룰렛 UI를 동시에 재생
		for (UCPRouletteWidget* Widget : Widgets)
		{
			if (Widget)
			{
				Widget->PlaySpin(ResultIndex, Slots.Num());
			}
		}
	}
	else
	{
		// UI 없이도 결과 처리는 그대로 동작하도록 하는 폴백
		HandleRouletteResultDetermined(ResultIndex);
	}

	return true;
}

TArray<UCPRouletteWidget*> ACPRoulette::GetOrCreateRouletteWidgets()
{
	if (!RouletteWidgetClass)
	{
		return TArray<UCPRouletteWidget*>();
	}

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC || !PC->IsLocalController())
			{
				continue;
			}

			const bool bAlreadyHasWidget = RouletteWidgetInstances.ContainsByPredicate([PC](const UCPRouletteWidget* Widget)
			{
				return Widget && Widget->GetOwningPlayer() == PC;
			});

			if (bAlreadyHasWidget)
			{
				continue;
			}

			if (UCPRouletteWidget* NewWidget = CreateWidget<UCPRouletteWidget>(PC, RouletteWidgetClass))
			{
				NewWidget->AddToViewport();
				NewWidget->OnResultDetermined.AddUniqueDynamic(this, &ACPRoulette::HandleRouletteResultDetermined);
				RouletteWidgetInstances.Add(NewWidget);
			}
		}
	}

	TArray<UCPRouletteWidget*> Widgets;
	Widgets.Reserve(RouletteWidgetInstances.Num());
	for (const TObjectPtr<UCPRouletteWidget>& Widget : RouletteWidgetInstances)
	{
		if (Widget)
		{
			Widgets.Add(Widget);
		}
	}

	return Widgets;
}

int32 ACPRoulette::PickWeightedSlotIndex() const
{
	float TotalProbability = 0.0f;
	for (const FCPRouletteSlotData& Slot : Slots)
	{
		TotalProbability += FMath::Max(Slot.Probability, 0.0f);
	}

	// 모든 칸의 Probability 합이 0 이하면(설정 실수 등) 균등 확률로 대체
	if (TotalProbability <= 0.0f)
	{
		return FMath::RandRange(0, Slots.Num() - 1);
	}

	float RemainingWeight = FMath::FRandRange(0.0f, TotalProbability);
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		RemainingWeight -= FMath::Max(Slots[Index].Probability, 0.0f);
		if (RemainingWeight <= 0.0f)
		{
			return Index;
		}
	}

	// 부동소수점 오차로 끝까지 못 뽑은 경우 첫번째 칸으로 대체
	return 0;
}

void ACPRoulette::HandleRouletteResultDetermined(int32 ResultIndex)
{
	// 스플릿 스크린 위젯마다 각자 OnResultDetermined를 브로드캐스트하므로,
	// 같은 스핀 결과에 대해 당첨 정보가 중복 전달되지 않도록 최초 1회만 처리
	if (!bIsRolling)
	{
		return;
	}

	bIsRolling = false;

	if (!Slots.IsValidIndex(ResultIndex))
	{
		return;
	}

	DeliverSlotReward(Slots[ResultIndex]);
}

void ACPRoulette::DeliverSlotReward(const FCPRouletteSlotData& SlotData)
{
	UE_LOG(LogTemp, Warning, TEXT("[ACPRoulette] Roulette Result - ItemID: %s, SpawnCount: %d, CoinType: %s, RewardTarget: %s"),
		*SlotData.ItemID.ToString(), SlotData.SpawnCount, *UEnum::GetValueAsString(SlotData.CoinType), *UEnum::GetValueAsString(SlotData.RewardTarget));

	switch (SlotData.RewardTarget)
	{
	case ECPRouletteRewardTarget::CoinPusher:
		if (CoinPusher)
		{
			CoinPusher->ItemSpawn(SlotData.ItemID, SlotData.SpawnCount, SlotData.CoinType);
		}
		break;

	case ECPRouletteRewardTarget::GameMode:
		if (ICPRouletteRewardReceiver* Receiver = GetWorld() ? Cast<ICPRouletteRewardReceiver>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			Receiver->ReceiveRouletteReward(SlotData.ItemID, SlotData.SpawnCount, SlotData.CoinType);
		}
		break;

	default:
		break;
	}
}

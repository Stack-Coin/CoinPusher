// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPRoulette.h"
#include "CPRouletteWidget.h"
#include "../CoinPusher/CPCoinPusher.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Player/CPGameMode.h"

ACPRoulette::ACPRoulette()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));

	Slots.SetNum(NumSlots);
}

bool ACPRoulette::Roll()
{
	// 이미 스핀 중이면(다른 플레이어가 먼저 돌린 경우 포함) 무시 - 하나의 룰렛을 두 플레이어가 공유
	if (bIsRolling || Slots.Num() == 0)
	{
		return false;
	}

	// GameMode가 관리하는 티켓을 1개 소모해야 스핀이 시작됨 - 부족하면 아무 일도 일어나지 않음
	ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr;
	if (!GameMode || !GameMode->TrySpendTeamTicket(1))
	{
		return false;
	}

	bIsRolling = true;

	const int32 ResultIndex = FMath::RandRange(0, Slots.Num() - 1);

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

void ACPRoulette::HandleRouletteResultDetermined(int32 ResultIndex)
{
	// 스플릿 스크린 위젯마다 각자 OnResultDetermined를 브로드캐스트하므로,
	// 같은 스핀 결과에 대해 아이템이 중복 스폰되지 않도록 최초 1회만 처리
	if (!bIsRolling)
	{
		return;
	}

	bIsRolling = false;

	if (!Slots.IsValidIndex(ResultIndex))
	{
		return;
	}

	SpawnSlotItems(Slots[ResultIndex]);
}

void ACPRoulette::SpawnSlotItems(const FCPRouletteSlotData& SlotData)
{
	if (!CoinPusher)
	{
		return;
	}

	CoinPusher->ItemSpawn(SlotData.ItemID, SlotData.SpawnCount);
}

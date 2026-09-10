// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPRoulette.h"
#include "CPRouletteWidget.h"
#include "../Datatables/CPItemData.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Player/CPGameMode.h"

namespace
{
	/** ItemDataTable에서 bRoulette인 행 하나를 추첨하기 위한 내부 후보 정보 - 헤더에 노출할 필요 없는
	 *  PickWeightedItem()만의 구현 세부사항이라 .cpp 익명 네임스페이스에 둔다 */
	struct FCPRouletteCandidate
	{
		FName ItemID;
		float Probability = 0.0f;
		int32 SpawnCount = 1;
	};
}

ACPRoulette::ACPRoulette()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
}

bool ACPRoulette::Roll()
{
	// 이미 스핀 중이면(다른 플레이어가 먼저 돌린 경우 포함) 무시 - 하나의 룰렛을 두 플레이어가 공유
	if (bIsRolling)
	{
		return false;
	}

	int32 ResultIndex = 0;
	int32 CandidateCount = 0;
	if (!PickWeightedItem(ResultIndex, CandidateCount))
	{
		return false;
	}

	bIsRolling = true;

	const TArray<UCPRouletteWidget*> Widgets = GetOrCreateRouletteWidgets();
	if (Widgets.Num() > 0)
	{
		// 로컬 스플릿 스크린의 모든 플레이어 화면에 동일한 룰렛 UI를 동시에 재생
		for (UCPRouletteWidget* Widget : Widgets)
		{
			if (Widget)
			{
				Widget->PlaySpin(ResultIndex, CandidateCount);
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

bool ACPRoulette::PickWeightedItem(int32& OutResultIndex, int32& OutCandidateCount)
{
	if (!ItemDataTable)
	{
		return false;
	}

	TArray<FCPRouletteCandidate> Candidates;
	float TotalProbability = 0.0f;

	for (const TPair<FName, uint8*>& RowPair : ItemDataTable->GetRowMap())
	{
		const FItemData* Row = reinterpret_cast<const FItemData*>(RowPair.Value);
		if (!Row || !Row->bRoulette)
		{
			continue;
		}

		FCPRouletteCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.ItemID = RowPair.Key;
		Candidate.Probability = FMath::Max(Row->RouletteProbability, 0.0f);
		Candidate.SpawnCount = Row->RouletteSpawnCount;

		TotalProbability += Candidate.Probability;
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	int32 PickedIndex = Candidates.Num() - 1;
	if (TotalProbability <= 0.0f)
	{
		// 모든 후보의 확률이 0 이하면(설정 실수 등) 균등 확률로 대체
		PickedIndex = FMath::RandRange(0, Candidates.Num() - 1);
	}
	else
	{
		float RemainingWeight = FMath::FRandRange(0.0f, TotalProbability);
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			RemainingWeight -= Candidates[Index].Probability;
			if (RemainingWeight <= 0.0f)
			{
				PickedIndex = Index;
				break;
			}
		}
	}

	PendingResultItemID = Candidates[PickedIndex].ItemID;
	PendingResultSpawnCount = Candidates[PickedIndex].SpawnCount;

	OutResultIndex = PickedIndex;
	OutCandidateCount = Candidates.Num();
	return true;
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

	UE_LOG(LogTemp, Warning, TEXT("[ACPRoulette] Roulette Result - ItemID: %s, SpawnCount: %d"),
		*PendingResultItemID.ToString(), PendingResultSpawnCount);

	OnPickedUp.Broadcast(PendingResultItemID, PendingResultSpawnCount);
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPRoulette.h"
#include "CPRouletteWidget.h"
#include "../Datatables/CPRouletteDataTypes.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Player/CPGameMode.h"
#include "Player/CPTopDownPlayerController.h"
#include "UI/CPInGameWidget.h"
#include "Log/CPLogCategories.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** RouletteDataTable의 행 하나를 추첨하기 위한 내부 후보 정보 - 헤더에 노출할 필요 없는
	 *  PickWeightedItem()만의 구현 세부사항이라 .cpp 익명 네임스페이스에 둔다 */
	struct FCPRouletteCandidate
	{
		FName ItemID;
		int32 SpawnCount = 1;
		int32 MustPickLevel = 0;
		UTexture2D* PickUpImage = nullptr;
	};

	/** FCPRouletteProbabilityRow는 레벨당 최대 10개 후보(Roulette_index0~9)까지만 가중치를 갖는
	 *  고정 컬럼 구조라, 인덱스로 필드를 조회하려면 이렇게 switch로 매핑해줘야 한다 */
	float GetProbabilityForIndex(const FCPRouletteProbabilityRow& Row, int32 Index)
	{
		switch (Index)
		{
		case 0: return Row.Roulette_index0;
		case 1: return Row.Roulette_index1;
		case 2: return Row.Roulette_index2;
		case 3: return Row.Roulette_index3;
		case 4: return Row.Roulette_index4;
		case 5: return Row.Roulette_index5;
		case 6: return Row.Roulette_index6;
		case 7: return Row.Roulette_index7;
		case 8: return Row.Roulette_index8;
		case 9: return Row.Roulette_index9;
		default: return 0.0f;
		}
	}
}

ACPRoulette::ACPRoulette()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
}

bool ACPRoulette::Roll(int32 PlayerLevel)
{
	// 이미 스핀 중이면(다른 플레이어가 먼저 돌린 경우 포함) 무시 - 하나의 룰렛을 두 플레이어가 공유
	if (bIsRolling)
	{
		return false;
	}

	int32 ResultIndex = 0;
	int32 CandidateCount = 0;
	if (!PickWeightedItem(PlayerLevel, ResultIndex, CandidateCount))
	{
		return false;
	}

	bIsRolling = true;

	if (RollSound)
	{
		UGameplayStatics::PlaySound2D(this, RollSound, RollSoundVolume);
	}

	const TArray<UCPRouletteWidget*> Widgets = GetLocalRouletteWidgets();
	if (Widgets.Num() > 0)
	{
		// 로컬 스플릿 스크린의 모든 플레이어 화면에 동일한 룰렛 UI를 동시에 재생
		for (UCPRouletteWidget* Widget : Widgets)
		{
			if (Widget)
			{
				Widget->PlaySpin(ResultIndex, CandidateCount, PendingResultPickUpImage);
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

TArray<UCPRouletteWidget*> ACPRoulette::GetLocalRouletteWidgets()
{
	TArray<UCPRouletteWidget*> Widgets;

	UWorld* World = GetWorld();
	if (!World)
	{
		return Widgets;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ACPTopDownPlayerController* PC = Cast<ACPTopDownPlayerController>(It->Get());
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}

		UCPInGameWidget* InGameWidget = PC->GetInGameWidget();
		UCPRouletteWidget* RouletteWidget = InGameWidget ? InGameWidget->GetRouletteWidget() : nullptr;
		if (!RouletteWidget)
		{
			continue;
		}

		// 매번 다시 호출해도 안전(AddUniqueDynamic) - InGameUI 인스턴스가 바뀌었을 수 있으므로
		// 캐싱하지 않고 매 Roll()마다 새로 조회한다
		RouletteWidget->OnResultDetermined.AddUniqueDynamic(this, &ACPRoulette::HandleRouletteResultDetermined);
		Widgets.Add(RouletteWidget);
	}

	return Widgets;
}

bool ACPRoulette::PickWeightedItem(int32 PlayerLevel, int32& OutResultIndex, int32& OutCandidateCount)
{
	if (!RouletteDataTable)
	{
		return false;
	}

	// RouletteDataTable의 행 나열 순서 = 추첨 인덱스(0..N-1). GetRowMap()이 아니라 GetRowNames()로
	// 순서를 얻어야 DataTable에 정의된 행 순서가 그대로 보존된다 (RouletteProbabilityDataTable의
	// Roulette_indexN이 이 순서를 가리키므로 순서가 바뀌면 안 됨)
	TArray<FCPRouletteCandidate> Candidates;
	for (const FName& RowName : RouletteDataTable->GetRowNames())
	{
		const FCPRouletteDataRow* Row = RouletteDataTable->FindRow<FCPRouletteDataRow>(RowName, TEXT("PickWeightedItem"));
		if (!Row)
		{
			continue;
		}

		FCPRouletteCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.ItemID = Row->ItemID;
		Candidate.SpawnCount = Row->PickEA;
		Candidate.MustPickLevel = Row->MustPickLevel;
		Candidate.PickUpImage = Row->PickUpImage;
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	// PlayerLevel과 MustPickLevel이 같은 행이 있으면, 그 행(들) 중에서 균등 확률로 반드시 하나를 당첨시킨다
	TArray<int32> MustPickIndices;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (Candidates[Index].MustPickLevel == PlayerLevel)
		{
			MustPickIndices.Add(Index);
		}
	}

	int32 PickedIndex;
	if (MustPickIndices.Num() > 0)
	{
		PickedIndex = MustPickIndices[FMath::RandRange(0, MustPickIndices.Num() - 1)];
	}
	else
	{
		// RouletteProbabilityDataTable에서 Level이 PlayerLevel과 같은 행을 찾아 인덱스별 가중치로 추첨.
		// 일치하는 Level 행이 없으면 가장 마지막 행(테이블에 정의된 순서 기준)의 가중치를 그대로 사용한다
		const FCPRouletteProbabilityRow* ProbabilityRow = nullptr;
		if (RouletteProbabilityDataTable)
		{
			const TArray<FName>& ProbabilityRowNames = RouletteProbabilityDataTable->GetRowNames();
			for (const FName& RowName : ProbabilityRowNames)
			{
				const FCPRouletteProbabilityRow* Row = RouletteProbabilityDataTable->FindRow<FCPRouletteProbabilityRow>(RowName, TEXT("PickWeightedItem"));
				if (Row && Row->Level == PlayerLevel)
				{
					ProbabilityRow = Row;
					break;
				}
			}

			if (!ProbabilityRow && ProbabilityRowNames.Num() > 0)
			{
				ProbabilityRow = RouletteProbabilityDataTable->FindRow<FCPRouletteProbabilityRow>(ProbabilityRowNames.Last(), TEXT("PickWeightedItem"));
			}
		}

		float TotalWeight = 0.0f;
		TArray<float> Weights;
		Weights.SetNum(Candidates.Num());
		if (ProbabilityRow)
		{
			for (int32 Index = 0; Index < Candidates.Num(); ++Index)
			{
				Weights[Index] = FMath::Max(GetProbabilityForIndex(*ProbabilityRow, Index), 0.0f);
				TotalWeight += Weights[Index];
			}
		}

		if (TotalWeight <= 0.0f)
		{
			// RouletteProbabilityDataTable이 비어있거나, 가중치 합이 0 이하면(설정 실수 등) 균등 확률로 대체
			PickedIndex = FMath::RandRange(0, Candidates.Num() - 1);
		}
		else
		{
			PickedIndex = Candidates.Num() - 1;
			float RemainingWeight = FMath::FRandRange(0.0f, TotalWeight);
			for (int32 Index = 0; Index < Candidates.Num(); ++Index)
			{
				RemainingWeight -= Weights[Index];
				if (RemainingWeight <= 0.0f)
				{
					PickedIndex = Index;
					break;
				}
			}
		}
	}

	PendingResultItemID = Candidates[PickedIndex].ItemID;
	PendingResultSpawnCount = Candidates[PickedIndex].SpawnCount;
	PendingResultPickUpImage = Candidates[PickedIndex].PickUpImage;

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

	UE_LOG(LogRoulette, Warning, TEXT("[ACPRoulette] Roulette Result - ItemID: %s, SpawnCount: %d"),
		*PendingResultItemID.ToString(), PendingResultSpawnCount);

	OnPickedUp.Broadcast(PendingResultItemID, PendingResultSpawnCount);
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPAugmentSelectionWidget.h"
#include "Player/CPAugmentCardWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UCPAugmentSelectionWidget::OpenAugmentSelection(TScriptInterface<ICPStatInterface> InStatTarget)
{
	StatTarget = InStatTarget;

	if (!CardContainer || !CardWidgetClass || AugmentPool.Num() == 0)
	{
		return;
	}

	CardContainer->ClearChildren();

	// Shuffle a working copy so the pool itself is never reordered, then take the first N as the offer.
	TArray<FCPAugmentData> Pool = AugmentPool;
	const int32 CardCount = FMath::Min(NumCardsToOffer, Pool.Num());

	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const int32 RandomIndex = FMath::RandRange(Index, Pool.Num() - 1);
		Pool.Swap(Index, RandomIndex);

		UCPAugmentCardWidget* Card = CreateWidget<UCPAugmentCardWidget>(this, CardWidgetClass);
		if (!Card)
		{
			continue;
		}

		Card->SetAugmentData(Pool[Index]);
		Card->OnCardSelected.AddDynamic(this, &UCPAugmentSelectionWidget::HandleCardSelected);

		if (UHorizontalBoxSlot* CardSlot = CardContainer->AddChildToHorizontalBox(Card))
		{
			CardSlot->SetPadding(FMargin(CardSpacing * 0.5f, 0.0f));
			CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CardSlot->SetHorizontalAlignment(HAlign_Center);
			CardSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UCPAugmentSelectionWidget::HandleCardSelected(const FCPAugmentData& SelectedAugment)
{
	ApplyAugment(SelectedAugment);

	if (CardContainer)
	{
		CardContainer->ClearChildren();
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UCPAugmentSelectionWidget::ApplyAugment(const FCPAugmentData& Augment)
{
	ICPStatInterface* Interface = StatTarget.GetInterface();
	if (!Interface)
	{
		return;
	}

	for (const FCPAugmentEffect& Effect : Augment.Effects)
	{
		const float CurrentValue = Interface->GetStat(Effect.StatType);
		const float Delta = CurrentValue * (Effect.PercentChange / 100.0f);
		Interface->ModifyStat(Effect.StatType, Delta);
	}
}

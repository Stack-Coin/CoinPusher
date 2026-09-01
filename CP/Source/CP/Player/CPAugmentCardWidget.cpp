// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPAugmentCardWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCPAugmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CardButton)
	{
		CardButton->OnClicked.AddDynamic(this, &UCPAugmentCardWidget::HandleClicked);
	}
}

void UCPAugmentCardWidget::SetAugmentData(const FCPAugmentData& InAugmentData)
{
	AugmentData = InAugmentData;

	if (CardText)
	{
		FString DisplayText = AugmentData.AugmentName.ToString();

		if (!AugmentData.Description.IsEmpty())
		{
			if (!DisplayText.IsEmpty())
			{
				DisplayText += TEXT("\n");
			}
			DisplayText += AugmentData.Description.ToString();
		}

		CardText->SetText(FText::FromString(DisplayText));
	}
}

void UCPAugmentCardWidget::HandleClicked()
{
	OnCardSelected.Broadcast(AugmentData);
}

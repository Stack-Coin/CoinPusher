// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPTimeDisplayWidget.h"
#include "Components/TextBlock.h"

void UCPTimeDisplayWidget::UpdateTime(float TimeInSeconds)
{
	if (!TimeText)
	{
		return;
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt32(TimeInSeconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	TimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}

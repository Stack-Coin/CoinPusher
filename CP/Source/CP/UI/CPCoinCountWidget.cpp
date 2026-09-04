// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCoinCountWidget.h"
#include "Components/TextBlock.h"

void UCPCoinCountWidget::UpdateCoinCount(int32 Count)
{
	if (CoinCountText)
	{
		CoinCountText->SetText(FText::Format(DisplayFormat, FText::AsNumber(Count)));
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCoinComboWidget.h"
#include "UI/CPHorizonGuageBarWidget.h"
#include "Components/TextBlock.h"

void UCPCoinComboWidget::SetComboCount(int32 Count)
{
	if (ComboCountText)
	{
		ComboCountText->SetText(FText::Format(DisplayFormat, FText::AsNumber(Count)));
	}
}

void UCPCoinComboWidget::UpdateComboGauge(float CurrentValue, float MaxValue)
{
	if (ComboGaugeWidget)
	{
		ComboGaugeWidget->Update(CurrentValue, MaxValue);
	}
}

void UCPCoinComboWidget::SetComboGaugeText(const FText& Text)
{
	if (ComboGaugeWidget)
	{
		ComboGaugeWidget->SetValueText(Text);
	}
}

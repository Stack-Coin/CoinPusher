// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPHorizonGuageBarWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCPHorizonGuageBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (FillImage)
	{
		// 왼쪽 끝을 기준점으로 고정해서, RenderScale.X가 줄어들 때 오른쪽에서 왼쪽으로 줄어들게 한다
		FillImage->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	}
}

void UCPHorizonGuageBarWidget::Update(float CurrentValue, float MaxValue)
{
	const float Percent = MaxValue > 0.0f ? FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f) : 0.0f;

	SetPercent(Percent);
	SetValues(CurrentValue, MaxValue);
}

void UCPHorizonGuageBarWidget::SetPercent_Implementation(float Percent)
{
	if (FillImage)
	{
		FillImage->SetRenderScale(FVector2D(Percent, 1.0f));
	}
}

void UCPHorizonGuageBarWidget::SetValues_Implementation(float CurrentValue, float MaxValue)
{
	if (ValueText)
	{
		ValueText->SetText(FText::Format(DisplayFormat, FText::AsNumber(CurrentValue), FText::AsNumber(MaxValue)));
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPHealthBarWidget.h"
#include "Components/Image.h"

void UCPHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (FillImage)
	{
		// 왼쪽 끝을 기준점으로 고정해서, RenderScale.X가 줄어들 때 오른쪽에서 왼쪽으로 줄어들게 한다
		FillImage->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	}
}

void UCPHealthBarWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	const float Percent = MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;

	SetHealthPercent(Percent);
	SetHealthValues(CurrentHealth, MaxHealth);
}

void UCPHealthBarWidget::SetHealthPercent_Implementation(float Percent)
{
	if (FillImage)
	{
		FillImage->SetRenderScale(FVector2D(Percent, 1.0f));
	}
}

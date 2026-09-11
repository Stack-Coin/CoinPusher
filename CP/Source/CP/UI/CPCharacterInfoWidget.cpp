// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCharacterInfoWidget.h"
#include "UI/CPHorizonGuageBarWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCPCharacterInfoWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (HealthGaugeWidget)
	{
		HealthGaugeWidget->Update(CurrentHealth, MaxHealth);
	}
}

void UCPCharacterInfoWidget::UpdateExp(float CurrentExp, float MaxExp)
{
	if (ExpGaugeWidget)
	{
		ExpGaugeWidget->Update(CurrentExp, MaxExp);
	}
}

void UCPCharacterInfoWidget::SetHealthText(const FText& Text)
{
	if (HealthGaugeWidget)
	{
		HealthGaugeWidget->SetValueText(Text);
	}
}

void UCPCharacterInfoWidget::SetExpText(const FText& Text)
{
	if (ExpGaugeWidget)
	{
		ExpGaugeWidget->SetValueText(Text);
	}
}

void UCPCharacterInfoWidget::SetCharacterName(const FText& CharacterName)
{
	if (NameText)
	{
		NameText->SetText(CharacterName);
	}
}

void UCPCharacterInfoWidget::SetLevel(int32 Level)
{
	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelDisplayFormat, FText::AsNumber(Level)));
	}
}

void UCPCharacterInfoWidget::SetPortrait(UTexture2D* Portrait)
{
	if (PortraitImage)
	{
		PortraitImage->SetBrushFromTexture(Portrait, false);
	}
}

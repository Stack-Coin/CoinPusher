// Fill out your copyright notice in the Description page of Project Settings.


#include "Nexus/CPUserWidget_NexusHpBar.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"

bool UCPUserWidget_NexusHpBar::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	HpProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PbHpBar"));
	WidgetTree->RootWidget = HpProgressBar;
	HpProgressBar->SetFillColorAndOpacity(FLinearColor(0.05f, 0.8f, 0.1f));

	return true;
}

void UCPUserWidget_NexusHpBar::UpdateHpBar(float NewCurrentHp)
{
	if (HpProgressBar)
	{
		const float HpPercent = MaxHp > 0.f ? NewCurrentHp / MaxHp : 0.f;
		HpProgressBar->SetPercent(FMath::Clamp(HpPercent, 0.f, 1.f));
	}
}

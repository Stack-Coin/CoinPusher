// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPItemToastWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UCPItemToastWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

void UCPItemToastWidget::ShowToast(const FCPItemData& ItemData)
{
	if (ItemNameText)
	{
		ItemNameText->SetText(ItemData.ItemName);
	}

	if (ItemDescriptionText)
	{
		ItemDescriptionText->SetText(ItemData.Description);
	}

	SetVisibility(ESlateVisibility::Visible);

	GetWorld()->GetTimerManager().SetTimer(HideTimerHandle, this, &UCPItemToastWidget::HideToast, ToastDisplayDuration, false);
}

void UCPItemToastWidget::HideToast()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

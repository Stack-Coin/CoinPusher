// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPEndingWidget.h"
#include "Components/Image.h"

void UCPEndingWidget::ShowResult(bool bIsClear)
{
	if (ClearImage)
	{
		ClearImage->SetVisibility(bIsClear ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (LoseImage)
	{
		LoseImage->SetVisibility(bIsClear ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCoinPointTextWidget.h"
#include "Components/TextBlock.h"

void UCPCoinPointTextWidget::SetPointText(const FText& InText)
{
	if (PointText)
	{
		PointText->SetText(InText);
	}
}

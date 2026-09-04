// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPTicketCountWidget.h"
#include "Components/TextBlock.h"

void UCPTicketCountWidget::UpdateTicketCount(int32 Count)
{
	if (TicketCountText)
	{
		TicketCountText->SetText(FText::Format(DisplayFormat, FText::AsNumber(Count)));
	}
}

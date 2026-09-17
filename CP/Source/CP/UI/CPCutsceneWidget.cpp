// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPCutsceneWidget.h"
#include "Kismet/GameplayStatics.h"

void UCPCutsceneWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OnAnyKeyPressed.AddDynamic(this, &UCPCutsceneWidget::HandleAnyKeyPressedForSkip);
}

void UCPCutsceneWidget::HandleAnyKeyPressedForSkip(FKey PressedKey)
{
	if (bHasRequestedLevelChange || NextLevelName.IsNone())
	{
		return;
	}

	bHasRequestedLevelChange = true;
	SwitchToNextWidget();
	//UGameplayStatics::OpenLevel(this, NextLevelName);
}

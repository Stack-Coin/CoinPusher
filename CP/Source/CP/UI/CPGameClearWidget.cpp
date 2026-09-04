// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPGameClearWidget.h"
#include "Kismet/GameplayStatics.h"

void UCPGameClearWidget::GoToNextLevel()
{
	if (!NextLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}

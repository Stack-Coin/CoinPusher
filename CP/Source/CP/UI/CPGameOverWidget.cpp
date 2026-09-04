// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPGameOverWidget.h"
#include "Kismet/GameplayStatics.h"

void UCPGameOverWidget::RestartLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

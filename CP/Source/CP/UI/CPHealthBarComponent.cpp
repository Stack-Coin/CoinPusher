// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPHealthBarComponent.h"
#include "UI/CPHorizonGuageBarWidget.h"

UCPHealthBarComponent::UCPHealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::World);
	SetDrawSize(FVector2D(150.0f, 20.0f));
	SetPivot(FVector2D(0.5f, 1.0f));
	SetTwoSided(true);
	SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
}

void UCPHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	HealthBarWidget = Cast<UCPHorizonGuageBarWidget>(GetUserWidgetObject());
}

void UCPHealthBarComponent::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (HealthBarWidget)
	{
		HealthBarWidget->Update(CurrentHealth, MaxHealth);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPRadialGaugeComponent.h"
#include "UI/CPRadialGaugeWidget.h"

UCPRadialGaugeComponent::UCPRadialGaugeComponent()
{
	SetWidgetSpace(EWidgetSpace::World);
	SetDrawSize(FVector2D(80.0f, 80.0f));
	SetPivot(FVector2D(0.5f, 0.5f));
	SetTwoSided(true);
	SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	SetRelativeRotation(FQuat(0.0f, 0.0f, 180.0f, 1.0f));
}

void UCPRadialGaugeComponent::BeginPlay()
{
	if (GaugeWidgetClass)
	{
		SetWidgetClass(GaugeWidgetClass);
	}

	Super::BeginPlay();

	GaugeWidget = Cast<UCPRadialGaugeWidget>(GetUserWidgetObject());
}

void UCPRadialGaugeComponent::UpdateGauge(float CurrentValue, float MaxValue)
{
	if (GaugeWidget)
	{
		GaugeWidget->UpdateGauge(CurrentValue, MaxValue);
	}
}

void UCPRadialGaugeComponent::SetGaugeEnabled(bool bEnabled)
{
	SetVisibility(bEnabled);
	SetActive(bEnabled);
}

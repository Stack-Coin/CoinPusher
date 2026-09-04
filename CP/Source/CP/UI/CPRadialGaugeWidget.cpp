// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPRadialGaugeWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

void UCPRadialGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!FillImage)
	{
		return;
	}

	// FillImage 브러시에 지정된 Material로부터 Dynamic Material Instance를 만들어서, 이후로는
	// 매번 새로 만들지 않고 Scalar Parameter만 갱신한다. 각도 기반 마스크는 그 Material이 담당
	if (UMaterialInterface* FillMaterial = Cast<UMaterialInterface>(FillImage->GetBrush().GetResourceObject()))
	{
		FillMaterialInstance = UMaterialInstanceDynamic::Create(FillMaterial, this);
		FillImage->SetBrushFromMaterial(FillMaterialInstance);
	}
}

void UCPRadialGaugeWidget::UpdateGauge(float CurrentValue, float MaxValue)
{
	const float Percent = MaxValue > 0.0f ? FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f) : 0.0f;

	SetGaugePercent(Percent);
}

void UCPRadialGaugeWidget::SetGaugePercent_Implementation(float Percent)
{
	if (FillMaterialInstance)
	{
		FillMaterialInstance->SetScalarParameterValue(PercentParameterName, Percent);
	}
}

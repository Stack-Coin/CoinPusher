// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPBuffIconWidget.h"
#include "Datatables/CPPlayerBuffData.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

void UCPBuffIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!MaskImage)
	{
		return;
	}

	// MaskImage 브러시에 지정된 Material로부터 Dynamic Material Instance를 만들어서, 이후로는
	// 매번 새로 만들지 않고 Scalar Parameter만 갱신한다 (UCPRadialGaugeWidget과 동일한 방식)
	if (UMaterialInterface* MaskMaterial = Cast<UMaterialInterface>(MaskImage->GetBrush().GetResourceObject()))
	{
		MaskMaterialInstance = UMaterialInstanceDynamic::Create(MaskMaterial, this);
		MaskImage->SetBrushFromMaterial(MaskMaterialInstance);
	}
}

void UCPBuffIconWidget::UpdateBuff(float CurrentTime, float MaxTime)
{
	if (CurrentTime <= 0.0f)
	{
		RemoveFromParent();
		return;
	}

	if (MaskMaterialInstance)
	{
		const float Percent = MaxTime > 0.0f ? FMath::Clamp(CurrentTime / MaxTime, 0.0f, 1.0f) : 0.0f;
		MaskMaterialInstance->SetScalarParameterValue(PercentParameterName, Percent);
	}

	if (CountDownText)
	{
		CountDownText->SetText(FText::Format(CountDownDisplayFormat, FText::AsNumber(FMath::CeilToInt(CurrentTime))));
	}
}

void UCPBuffIconWidget::SetBuffCode(FName NewBuffCode)
{
	BuffCode = NewBuffCode;

	if (!PlayerBuffDataTable || !BackgroundImage)
	{
		return;
	}

	const FCPPlayerBuffData* Row = PlayerBuffDataTable->FindRow<FCPPlayerBuffData>(BuffCode, TEXT("UCPBuffIconWidget::SetBuffCode"));
	if (Row && Row->BuffImage)
	{
		BackgroundImage->SetBrushFromTexture(Row->BuffImage, false);
	}
}

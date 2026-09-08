// Fill out your copyright notice in the Description page of Project Settings.

#include "CoinPusher/CPCoinPusherCaptureWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Layout/Anchors.h"
#include "Layout/Margin.h"

TSharedRef<SWidget> UCPCoinPusherCaptureWidget::RebuildWidget()
{
	CaptureBrush = MakeShared<FSlateBrush>();
	CaptureBrush->DrawAs = ESlateBrushDrawType::Image;
	CaptureBrush->Tiling = ESlateBrushTileType::NoTile;

	const float ClampedRatio = FMath::Clamp(CaptureWidthRatio, 0.0f, 1.0f);

	return SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.0f, 0.0f, ClampedRatio, 1.0f))
		.Offset(FMargin(0.0f))
		.Alignment(FVector2D(0.0f, 0.0f))
		[
			SAssignNew(CaptureImageWidget, SImage)
			.Image(CaptureBrush.Get())
		];
}

void UCPCoinPusherCaptureWidget::SetCaptureTexture(UTextureRenderTarget2D* InTexture)
{
	if (!CaptureBrush || !InTexture)
	{
		return;
	}

	CaptureBrush->SetResourceObject(InTexture);
	CaptureBrush->ImageSize = FVector2D(InTexture->SizeX, InTexture->SizeY);
}

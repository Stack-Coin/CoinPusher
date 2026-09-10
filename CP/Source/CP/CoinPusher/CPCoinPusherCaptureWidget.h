// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCoinPusherCaptureWidget.generated.h"

class SImage;
class UTextureRenderTarget2D;
struct FSlateBrush;

/**
 *  Shows a CoinPusher's SceneCaptureComponent2D render target (see UCPCoinPusherViewCaptureComponent) as a
 *  Picture-in-Picture overlay, anchored to the left CaptureWidthRatio of the screen. Builds its own Slate
 *  widget tree in RebuildWidget - no WBP asset needed, this class can be used directly (e.g.
 *  UCPCoinPusherCaptureWidget::StaticClass()). Whoever creates it (typically a PlayerController) adds it
 *  full-screen via AddToViewport and calls SetCaptureTexture with the source component's
 *  GetViewRenderTarget() - CaptureWidthRatio here should match UCPCoinPusherViewportClient's, since that's
 *  what shrinks the local player's own camera viewport to the remaining region so the two don't overlap.
 */
UCLASS()
class CP_API UCPCoinPusherCaptureWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Fraction (0-1) of the screen width, from the left edge, this widget's image is anchored to fill */
	UPROPERTY(EditAnywhere, Category="CoinPusher Picture-in-Picture", meta = (ClampMin = 0, ClampMax = 1))
	float CaptureWidthRatio = 0.3f;

	/** Brush drawing CaptureImageWidget - its resource object is set to the render target in SetCaptureTexture */
	TSharedPtr<FSlateBrush> CaptureBrush;

	/** The Slate image built and returned by RebuildWidget */
	TSharedPtr<SImage> CaptureImageWidget;

	/** Builds an SConstraintCanvas with a single SImage slot anchored to (0,0)-(CaptureWidthRatio,1) - the
	 *  Slate equivalent of a UMG Canvas Panel with an Image anchored to the left CaptureWidthRatio, without
	 *  needing an actual WBP asset */
	virtual TSharedRef<SWidget> RebuildWidget() override;

public:

	/** Points the displayed image at InTexture (typically a UCPCoinPusherViewCaptureComponent's render target) */
	void SetCaptureTexture(UTextureRenderTarget2D* InTexture);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "CPCoinPusherViewCaptureComponent.generated.h"

class UTextureRenderTarget2D;

/**
 *  SceneCaptureComponent2D for a CoinPusher Picture-in-Picture view. Attach to any actor (e.g. ACPCoinPusher,
 *  or a lightweight test actor under CoinPusher/Test) and position/rotate it in the editor like any other
 *  camera. TextureTarget (inherited from USceneCaptureComponent2D, EditAnywhere) can be assigned directly in
 *  the BP/Details panel to a Render Target asset created in the Content Browser - e.g. to build a Material
 *  from it and use that Material in a WBP. If TextureTarget is left unset, BeginPlay creates one at
 *  RenderTargetSize automatically instead, so this still works with zero setup (see
 *  ACPCoinPusherCaptureTestActor). Either way, GetViewRenderTarget() returns whichever one ends up in use.
 */
UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class CP_API UCPCoinPusherViewCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:

	UCPCoinPusherViewCaptureComponent();

protected:

	/** Fallback resolution for the render target auto-created in BeginPlay, used only if the actual game
	 *  viewport size couldn't be read. Ignored if TextureTarget is already assigned to a Render Target asset */
	UPROPERTY(EditAnywhere, Category="CoinPusher Picture-in-Picture", meta = (ClampMin = 4))
	FIntPoint RenderTargetSize = FIntPoint(512, 960);

	/** Fraction (0-1) of the screen width the PIP occupies - should match UCPCoinPusherViewportClient's own
	 *  CaptureWidthRatio. Used to size the auto-created render target to the actual on-screen pixel area
	 *  (see BeginPlay) so the image isn't upscaled and blurry/pixelated */
	UPROPERTY(EditAnywhere, Category="CoinPusher Picture-in-Picture", meta = (ClampMin = 0, ClampMax = 1))
	float CaptureWidthRatio = 0.3f;

	/** Multiplies the on-screen-matched resolution before creating the render target - the manual
	 *  equivalent of Screen Percentage/supersampling (FPostProcessSettings::ScreenPercentage is deprecated,
	 *  this is the practical replacement): rendering above 1:1 and letting the display downscale gives
	 *  smoother edges/less pixelation, at extra GPU cost. 1.0 = no supersampling, 1.5-2.0 = noticeably sharper */
	UPROPERTY(EditAnywhere, Category="CoinPusher Picture-in-Picture", meta = (ClampMin = 0.5, ClampMax = 4.0))
	float SupersampleFactor = 1.0f;

	/** Uses a 16-bit-float-per-channel render target (RTF_RGBA16f) instead of the default 8-bit
	 *  (RTF_RGBA8). Mainly reduces color banding in gradients/shadows at extra memory cost - it does NOT fix
	 *  blur/pixelation from upscaling (use SupersampleFactor or CaptureWidthRatio matching for that) */
	UPROPERTY(EditAnywhere, Category="CoinPusher Picture-in-Picture")
	bool bHighPrecisionColor = false;

	/** The render target actually in use - either TextureTarget as assigned in the BP/Details panel, or one
	 *  auto-created in BeginPlay if it was left unset */
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ViewRenderTarget;

	/** Respects a TextureTarget already assigned in the BP/Details panel; only auto-creates one (at
	 *  RenderTargetSize) when it was left unset */
	virtual void BeginPlay() override;

	/** Explicitly captures every tick (see constructor - bCaptureEveryFrame's own "every rendered frame"
	 *  bookkeeping doesn't reliably pick up a TextureTarget assigned after this component was already
	 *  registered, which is what BeginPlay does here) */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	/** Returns the render target this component captures into (valid after BeginPlay) */
	FORCEINLINE UTextureRenderTarget2D* GetViewRenderTarget() const { return ViewRenderTarget; }
};

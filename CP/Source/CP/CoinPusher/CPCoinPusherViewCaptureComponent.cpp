// Fill out your copyright notice in the Description page of Project Settings.

#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"

UCPCoinPusherViewCaptureComponent::UCPCoinPusherViewCaptureComponent()
{
	// bCaptureEveryFrame의 "매 렌더 프레임 자동 캡처"는 등록 시점에 TextureTarget이 이미 있어야
	// 제대로 편입되는 것으로 보인다 - 우리는 BeginPlay에서 등록 이후에 TextureTarget을 만들어 붙이기
	// 때문에 자동 캡처가 계속 안 일어나고, 프로퍼티를 건드려 재등록될 때만 한 번 캡처되는 증상이
	// 있었다. 그래서 자동 캡처에 기대지 않고 TickComponent에서 매 틱 직접 CaptureScene()을 호출한다
	PrimaryComponentTick.bCanEverTick = true;

	bCaptureEveryFrame = false;
	bCaptureOnMovement = false;
	CaptureSource = SCS_FinalColorLDR;

	// TAA 등 다른 temporal 효과도 프레임마다 리셋되지 않고 누적되도록 유지 (연속 캡처이므로 매 프레임
	// 리셋할 이유가 없다)
	bAlwaysPersistRenderingState = true;

	// 이 프로젝트는 Lumen GI/Reflection을 쓰는데(DefaultEngine.ini r.DynamicGlobalIlluminationMethod=1,
	// r.ReflectionMethod=1), SceneCaptureComponent는 기본적으로 bAlwaysPersistRenderingState=false라
	// Lumen이 여러 프레임에 걸쳐 누적해야 하는 temporal 데이터가 매 프레임 초기화되어 캡처가 계속
	// 검게(또는 수렴하지 않은 채로) 나온다. 캡처 자체의 GI/Reflection을 꺼서 Lumen 의존성을 없앤다 -
	// Directional Light 등 직접광은 GI 방식과 무관하게 그대로 나오므로 화면 한 켠의 작은 PIP에는 충분하다
	PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod = true;
	PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
	PostProcessSettings.bOverride_ReflectionMethod = true;
	PostProcessSettings.ReflectionMethod = EReflectionMethod::None;

	// SCS_FinalColorLDR는 Auto Exposure(눈 적응)의 영향을 받는데, 막 스폰된 캡처는 노출값이 수렴하기
	// 전이라 검게 나오거나 Post Process Volume이 없는 레벨에서는 계속 검게 나올 수 있다.
	// Min/MaxBrightness를 같은 값으로 고정해서 노출을 완전히 고정 - AEM_Manual과 달리 카메라의
	// 조리개/셔터스피드/ISO 기본값 조합에 좌우되지 않아 훨씬 안정적으로 밝기가 나온다
	PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	PostProcessSettings.AutoExposureMinBrightness = 1.0f;
	PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	PostProcessSettings.AutoExposureMaxBrightness = 1.0f;
}

void UCPCoinPusherViewCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	if (TextureTarget)
	{
		// BP/디테일 패널에서 이미 Render Target 에셋을 직접 지정해뒀으면 그대로 사용 - 덮어쓰지 않는다
		ViewRenderTarget = TextureTarget;
		return;
	}

	// 화면에 실제로 표시될 픽셀 해상도에 최대한 맞춰서 만든다 - 고정된 작은 해상도로 만들면 더 큰
	// 화면(고해상도 모니터)에서는 확대되면서 흐릿/블록(pixel화)하게 보인다
	FIntPoint ResolvedSize = RenderTargetSize;
	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D ViewportSize;
			ViewportClient->GetViewportSize(ViewportSize);

			if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
			{
				ResolvedSize.X = FMath::Max(4, FMath::RoundToInt(ViewportSize.X * FMath::Clamp(CaptureWidthRatio, 0.0f, 1.0f)));
				ResolvedSize.Y = FMath::Max(4, FMath::RoundToInt(ViewportSize.Y));
			}
		}
	}

	// SupersampleFactor > 1이면 화면보다 더 높은 해상도로 렌더링해서 화면에 표시될 때 다운스케일되며
	// 다운샘플링 안티에일리어싱 효과를 낸다 (Deprecated된 PostProcessSettings.ScreenPercentage의 대체)
	const float ClampedSupersample = FMath::Clamp(SupersampleFactor, 0.5f, 4.0f);
	ResolvedSize.X = FMath::Max(4, FMath::RoundToInt(ResolvedSize.X * ClampedSupersample));
	ResolvedSize.Y = FMath::Max(4, FMath::RoundToInt(ResolvedSize.Y * ClampedSupersample));

	ViewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	ViewRenderTarget->RenderTargetFormat = bHighPrecisionColor ? RTF_RGBA16f : RTF_RGBA8;
	ViewRenderTarget->Filter = TF_Bilinear;
	ViewRenderTarget->InitAutoFormat(ResolvedSize.X, ResolvedSize.Y);
	ViewRenderTarget->UpdateResourceImmediate(true);

	TextureTarget = ViewRenderTarget;
}

void UCPCoinPusherViewCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// BeginPlay 전(예: BP 에디터 자신의 프리뷰 뷰포트 - 여긴 게임플레이가 아니라서 BeginPlay가 호출되지
	// 않는다)에는 TextureTarget이 아직 없으므로 캡처를 시도하지 않는다
	if (!HasBegunPlay() || !TextureTarget)
	{
		return;
	}

	CaptureScene();
}

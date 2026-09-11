// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCoinPointUI.h"
#include "UI/CPCoinPointTextWidget.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraTypes.h"
#include "TimerManager.h"

void UCPCoinPointUI::ShowPointText(const FText& Text, FVector WorldLocation)
{
	if (!PointTextCanvas || !PointTextWidgetClass)
	{
		return;
	}

	FVector2D ViewportPosition;
	if (!CalculateClampedScreenPosition(WorldLocation, ViewportPosition))
	{
		return;
	}

	UCPCoinPointTextWidget* PointTextWidget = CreateWidget<UCPCoinPointTextWidget>(this, PointTextWidgetClass);
	if (!PointTextWidget)
	{
		return;
	}

	PointTextWidget->SetPointText(Text);

	if (UCanvasPanelSlot* CanvasSlot = PointTextCanvas->AddChildToCanvas(PointTextWidget))
	{
		CanvasSlot->SetPosition(ViewportPosition);
		CanvasSlot->SetAutoSize(true);
	}

	PointTextWidget->PlayAppearEffect();

	UWorld* World = GetWorld();
	if (DisplayDuration > 0.0f && World)
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UCPCoinPointUI::RemovePointText, PointTextWidget);
		World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, DisplayDuration, false);
	}
	else
	{
		RemovePointText(PointTextWidget);
	}
}

void UCPCoinPointUI::ShowCoinPointText(FVector WorldLocation)
{
	ShowPointText(CoinPointDisplayText, WorldLocation);
}

UCPCoinPusherViewCaptureComponent* UCPCoinPointUI::GetCaptureComponent() const
{
	if (UCPCoinPusherViewCaptureComponent* Cached = CachedCaptureComponent.Get())
	{
		return Cached;
	}

	if (ACPCoinPusher* CoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass())))
	{
		CachedCaptureComponent = CoinPusher->GetViewCaptureComponent();
	}

	return CachedCaptureComponent.Get();
}

bool UCPCoinPointUI::CalculateClampedScreenPosition(const FVector& WorldLocation, FVector2D& OutViewportPosition) const
{
	UCPCoinPusherViewCaptureComponent* CaptureComponent = GetCaptureComponent();
	if (!CaptureComponent)
	{
		return false;
	}

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ClampedRatio = FMath::Clamp(CaptureWidthRatio, 0.0f, 1.0f);
	const float PipWidth = ViewportSize.X * ClampedRatio;
	const float PipHeight = ViewportSize.Y;

	if (PipWidth <= 0.0f || PipHeight <= 0.0f)
	{
		return false;
	}

	const float AspectRatio = PipWidth / PipHeight;

	// 캡처 컴포넌트의 로컬 공간으로 변환: X=정면(깊이), Y=오른쪽, Z=위쪽 (액터/컴포넌트의 표준 UE 축 규약)
	const FVector ViewLocation = CaptureComponent->GetComponentLocation();
	const FRotator ViewRotation = CaptureComponent->GetComponentRotation();
	const FVector LocalOffset = ViewRotation.UnrotateVector(WorldLocation - ViewLocation);

	if (LocalOffset.X <= KINDA_SMALL_NUMBER)
	{
		// 캡처 카메라 뒤쪽 - 투영 좌표를 신뢰할 수 없으므로 좌/우 방향만 판단해 PIP 가장자리
		// (세로 중앙)에 표시
		OutViewportPosition = FVector2D(LocalOffset.Y >= 0.0f ? PipWidth - OffscreenMargin : OffscreenMargin, PipHeight * 0.5f);
		return true;
	}

	float NdcX;
	float NdcY;

	if (CaptureComponent->ProjectionType == ECameraProjectionMode::Orthographic)
	{
		const float HalfWidth = FMath::Max(CaptureComponent->OrthoWidth, 1.0f) * 0.5f;
		const float HalfHeight = HalfWidth / AspectRatio;
		NdcX = LocalOffset.Y / HalfWidth;
		NdcY = LocalOffset.Z / HalfHeight;
	}
	else
	{
		// FOVAngle은 UE 카메라 표준 규약상 수평 FOV - 수직 확장은 AspectRatio로 나눠서 구한다
		const float HalfFOVRadians = FMath::DegreesToRadians(FMath::Max(CaptureComponent->FOVAngle, 1.0f)) * 0.5f;
		const float HorizontalExtentAtDepth = LocalOffset.X * FMath::Tan(HalfFOVRadians);
		const float VerticalExtentAtDepth = HorizontalExtentAtDepth / AspectRatio;
		NdcX = LocalOffset.Y / HorizontalExtentAtDepth;
		NdcY = LocalOffset.Z / VerticalExtentAtDepth;
	}

	// NDC(-1~1, X=오른쪽+, Y=위쪽+)를 PIP 영역 픽셀 좌표(원점 좌상단, Y=아래쪽+)로 매핑
	FVector2D PipPosition;
	PipPosition.X = (NdcX * 0.5f + 0.5f) * PipWidth;
	PipPosition.Y = (1.0f - (NdcY * 0.5f + 0.5f)) * PipHeight;

	const bool bOnScreen = FMath::Abs(NdcX) <= 1.0f && FMath::Abs(NdcY) <= 1.0f;
	if (!bOnScreen)
	{
		PipPosition.X = FMath::Clamp(PipPosition.X, OffscreenMargin, PipWidth - OffscreenMargin);
		PipPosition.Y = FMath::Clamp(PipPosition.Y, OffscreenMargin, PipHeight - OffscreenMargin);
	}

	// PIP는 항상 화면 왼쪽 끝(Left=0)에서 시작하므로, PIP 로컬 픽셀 좌표가 곧 뷰포트 좌표
	OutViewportPosition = PipPosition;
	return true;
}

void UCPCoinPointUI::RemovePointText(UCPCoinPointTextWidget* PointTextWidget)
{
	if (PointTextWidget)
	{
		PointTextWidget->RemoveFromParent();
	}
}

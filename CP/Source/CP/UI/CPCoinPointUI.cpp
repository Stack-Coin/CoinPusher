// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCoinPointUI.h"
#include "UI/CPCoinPointTextWidget.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPDropZone.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Log/CPLogCategories.h"

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

	UE_LOG(LogUI, Log, TEXT("[UCPCoinPointUI] Dropped at WorldLocation=%s -> Displayed at ViewportPosition=%s"),
		*WorldLocation.ToString(), *ViewportPosition.ToString());

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

ACPDropZone* UCPCoinPointUI::GetDropZone() const
{
	if (ACPDropZone* Cached = CachedDropZone.Get())
	{
		return Cached;
	}

	if (ACPCoinPusher* CoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass())))
	{
		CachedDropZone = CoinPusher->GetDropZone();
	}

	return CachedDropZone.Get();
}

bool UCPCoinPointUI::CalculateClampedScreenPosition(const FVector& WorldLocation, FVector2D& OutViewportPosition) const
{
	ACPDropZone* DropZone = GetDropZone();
	const UBoxComponent* CollectionVolume = DropZone ? DropZone->GetCollectionVolume() : nullptr;
	if (!DropZone || !CollectionVolume || !PointTextCanvas)
	{
		return false;
	}

	const FVector2D AreaSize = PointTextCanvas->GetCachedGeometry().GetLocalSize();
	if (AreaSize.X <= 0.0f || AreaSize.Y <= 0.0f)
	{
		return false;
	}

	// DropZone의 CollectionVolume Box Extent를 "정해진 영역" 기준으로 삼아, WorldLocation의 DropZone
	// 로컬 오프셋(Z=가로, X=세로)을 -1~1로 정규화한다 - 카메라 투영은 쓰지 않는 단순 선형 매핑
	const FVector BoxExtent = CollectionVolume->GetScaledBoxExtent();
	const FVector LocalOffset = WorldLocation - DropZone->GetActorLocation();

	const float NormalizedX = BoxExtent.Z > KINDA_SMALL_NUMBER ? FMath::Clamp(LocalOffset.Z / BoxExtent.Z, -1.0f, 1.0f) : 0.0f;
	const float NormalizedY = BoxExtent.X > KINDA_SMALL_NUMBER ? FMath::Clamp(LocalOffset.X / BoxExtent.X, -1.0f, 1.0f) : 0.0f;

	// -1~1 정규화 값을 PointTextCanvas 픽셀 좌표(원점 좌상단)로 매핑하되, 가장자리에 바짝 붙지
	// 않도록 OffscreenMargin만큼 안쪽으로 들여온 범위로 매핑한다
	const float ClampedMarginX = FMath::Min(OffscreenMargin, AreaSize.X * 0.5f);
	const float ClampedMarginY = FMath::Min(OffscreenMargin, AreaSize.Y * 0.5f);

	OutViewportPosition.X = FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(ClampedMarginX, AreaSize.X - ClampedMarginX), NormalizedX);
	OutViewportPosition.Y = FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(ClampedMarginY, AreaSize.Y - ClampedMarginY), NormalizedY);
	return true;
}

void UCPCoinPointUI::RemovePointText(UCPCoinPointTextWidget* PointTextWidget)
{
	if (PointTextWidget)
	{
		PointTextWidget->RemoveFromParent();
	}
}

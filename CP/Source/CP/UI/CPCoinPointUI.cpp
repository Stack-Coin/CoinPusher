// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPCoinPointUI.h"
#include "UI/CPCoinPointTextWidget.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "Datatables/CPItemData.h"
#include "Engine/DataTable.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraTypes.h"
#include "TimerManager.h"
#include "Log/CPLogCategories.h"

void UCPCoinPointUI::ShowPointText(const FText& Text, FVector WorldLocation)
{
	if (!PointTextCanvas || !PointTextWidgetClass)
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPCoinPointUI] ShowPointText 무시됨 - PointTextCanvas(%s)/PointTextWidgetClass(%s)가 WBP Class Defaults에 지정돼 있는지 확인하세요."),
			PointTextCanvas ? TEXT("OK") : TEXT("null"), PointTextWidgetClass ? TEXT("OK") : TEXT("null"));
		return;
	}

	FVector2D ViewportPosition;
	if (!CalculateClampedScreenPosition(WorldLocation, ViewportPosition))
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPCoinPointUI] ShowPointText 무시됨 - CalculateClampedScreenPosition 실패 (Screen Capture 카메라를 못 찾았거나 PointTextCanvas의 GetCachedGeometry()가 아직 0인 상태 - 위젯이 아직 화면에 한 번도 그려지지 않았을 수 있음)"));
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

void UCPCoinPointUI::ShowCoinPointText(FName ItemID, FVector WorldLocation)
{
	ShowPointText(ResolveCoinPointText(ItemID), WorldLocation);
}

ACPCoinPusher* UCPCoinPointUI::GetCoinPusher() const
{
	if (ACPCoinPusher* Cached = CachedCoinPusher.Get())
	{
		return Cached;
	}

	CachedCoinPusher = Cast<ACPCoinPusher>(UGameplayStatics::GetActorOfClass(this, ACPCoinPusher::StaticClass()));
	if (!CachedCoinPusher.IsValid())
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPCoinPointUI] GetCoinPusher - 레벨에서 ACPCoinPusher를 찾지 못했습니다."));
	}

	return CachedCoinPusher.Get();
}

UCPCoinPusherViewCaptureComponent* UCPCoinPointUI::GetCaptureComponent() const
{
	if (UCPCoinPusherViewCaptureComponent* Cached = CachedCaptureComponent.Get())
	{
		return Cached;
	}

	ACPCoinPusher* CoinPusher = GetCoinPusher();
	if (!CoinPusher)
	{
		return nullptr;
	}

	CachedCaptureComponent = CoinPusher->GetViewCaptureComponent();
	if (!CachedCaptureComponent.IsValid())
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPCoinPointUI] GetCaptureComponent - %s에 ViewCaptureComponent(Screen Capture 카메라)가 없습니다."),
			*GetNameSafe(CoinPusher));
	}

	return CachedCaptureComponent.Get();
}

FText UCPCoinPointUI::ResolveCoinPointText(FName ItemID) const
{
	ACPCoinPusher* CoinPusher = GetCoinPusher();
	UDataTable* ItemDataTable = CoinPusher ? CoinPusher->GetItemDataTable() : nullptr;
	if (!ItemDataTable)
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPCoinPointUI] ResolveCoinPointText - ItemDataTable을 찾을 수 없어 CoinPointDisplayText로 대체합니다 (ItemID=%s)."), *ItemID.ToString());
		return CoinPointDisplayText;
	}

	const FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UCPCoinPointUI::ResolveCoinPointText"));
	if (!Row || Row->CoinPointText.IsEmpty())
	{
		return CoinPointDisplayText;
	}

	return Row->CoinPointText;
}

bool UCPCoinPointUI::CalculateClampedScreenPosition(const FVector& WorldLocation, FVector2D& OutViewportPosition) const
{
	UCPCoinPusherViewCaptureComponent* CaptureComponent = GetCaptureComponent();
	if (!CaptureComponent || !PointTextCanvas)
	{
		return false;
	}

	const FVector2D AreaSize = PointTextCanvas->GetCachedGeometry().GetLocalSize();
	if (AreaSize.X <= 0.0f || AreaSize.Y <= 0.0f)
	{
		return false;
	}

	// 캡처 컴포넌트(Screen Capture 카메라)의 로컬 공간으로 변환: X=정면(깊이), Y=오른쪽, Z=위쪽
	// (액터/컴포넌트의 표준 UE 축 규약) - Player를 따라다니는 Main Camera가 아니라 코인 푸셔를
	// 비추는 이 카메라를 기준으로 좌우 위치를 계산해야 실제 화면에 보이는 좌우 위치와 일치한다
	const FVector ViewLocation = CaptureComponent->GetComponentLocation();
	const FRotator ViewRotation = CaptureComponent->GetComponentRotation();
	const FVector LocalOffset = ViewRotation.UnrotateVector(WorldLocation - ViewLocation);

	float NdcX;
	if (LocalOffset.X <= KINDA_SMALL_NUMBER)
	{
		// 카메라 뒤쪽 - 투영 좌표를 신뢰할 수 없으므로 좌/우 방향만 판단해 가장자리에 표시
		NdcX = LocalOffset.Y >= 0.0f ? 1.0f : -1.0f;
	}
	else if (CaptureComponent->ProjectionType == ECameraProjectionMode::Orthographic)
	{
		const float HalfWidth = FMath::Max(CaptureComponent->OrthoWidth, 1.0f) * 0.5f;
		NdcX = LocalOffset.Y / HalfWidth;
	}
	else
	{
		// FOVAngle은 UE 카메라 표준 규약상 수평 FOV
		const float HalfFOVRadians = FMath::DegreesToRadians(FMath::Max(CaptureComponent->FOVAngle, 1.0f)) * 0.5f;
		const float HorizontalExtentAtDepth = LocalOffset.X * FMath::Tan(HalfFOVRadians);
		NdcX = LocalOffset.Y / HorizontalExtentAtDepth;
	}

	// NDC(-1~1, 오른쪽+)를 PointTextCanvas 가로(X) 픽셀 좌표(원점 좌상단)로 매핑하되, 시야 밖이면
	// 가장자리에서 OffscreenMargin만큼 안쪽으로 들여온 위치로 클램프한다
	const float ClampedMarginX = FMath::Min(OffscreenMargin, AreaSize.X * 0.5f);
	const float RawX = (NdcX * 0.5f + 0.5f) * AreaSize.X;
	OutViewportPosition.X = FMath::Abs(NdcX) <= 1.0f ? RawX : FMath::Clamp(RawX, ClampedMarginX, AreaSize.X - ClampedMarginX);

	// 세로(Y)는 떨어진 위치와 무관하게 항상 FixedVerticalRatio 비율 고정 위치에 즉시 뜨도록 함
	// (드랍 깊이에 따라 위아래로 흩어지지 않고, 가로 위치만 드랍 위치를 따라가게 하기 위함)
	const float ClampedMarginY = FMath::Min(OffscreenMargin, AreaSize.Y * 0.5f);
	OutViewportPosition.Y = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(ClampedMarginY, AreaSize.Y - ClampedMarginY), FMath::Clamp(FixedVerticalRatio, 0.0f, 1.0f));
	return true;
}

void UCPCoinPointUI::RemovePointText(UCPCoinPointTextWidget* PointTextWidget)
{
	if (PointTextWidget)
	{
		PointTextWidget->RemoveFromParent();
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCoinPointUI.generated.h"

class ACPCoinPusher;
class UCanvasPanel;
class UCPCoinPointTextWidget;
class UCPCoinPusherViewCaptureComponent;

/**
 *  코인이 떨어진 월드 위치를 화면 좌표로 계산해서, 그 자리에 안내 문구(예: "+1")를 잠깐 띄웠다가
 *  사라지게 하는 UI. UCPInGameWidget("InGameUI")이 컴포넌트로 갖고 있으며, 실제 코인 드랍 이벤트는
 *  ACPDropZone::OnCoinDropped(FVector) 델리게이트를 이 위젯의 ShowCoinPointText에 그대로
 *  Bind Event해서 받는다 (그 외 임의의 문구를 원하는 위치에 띄우고 싶으면 ShowPointText(Text,
 *  Location)를 직접 호출).
 *
 *  코인이 실제로 눈에 보이는 화면은 플레이어의 메인 게임플레이 카메라가 아니라, ACPCoinPusher의
 *  Picture-in-Picture(UCPCoinPusherViewCaptureComponent, SceneCaptureComponent2D)가 캡처해서
 *  화면 왼쪽 CaptureWidthRatio 영역에 그리는 별도의 뷰다(CoinPusher/CPCoinPusherViewCaptureComponent.h,
 *  CPCoinPusherCaptureWidget.h, CPCoinPusherViewportClient.h 참고 - 플레이어 자신의 카메라
 *  뷰포트는 오른쪽 (1-CaptureWidthRatio) 영역으로 축소되어 있다). 그래서 이 위젯은 플레이어
 *  컨트롤러의 카메라가 아니라 레벨에 배치된 ACPCoinPusher::GetViewCaptureComponent()의 위치/회전/
 *  FOV(또는 Ortho Width)를 직접 이용해 WorldLocation을 그 캡처 화면 기준으로 투영하고, 그 결과를
 *  화면 왼쪽 PIP 영역(가로 CaptureWidthRatio, 세로 전체)의 픽셀 좌표로 매핑한다.
 *
 *  PointTextCanvas(UCanvasPanel, BindWidgetOptional) 위에 PointTextWidgetClass
 *  (UCPCoinPointTextWidget 상속 WBP)의 인스턴스를 호출마다 하나씩 생성해 계산된 화면 좌표에
 *  배치하고, DisplayDuration 후 제거한다 - 동시에 여러 코인이 떨어져도 각자 독립된 인스턴스로
 *  겹쳐 표시된다.
 *
 *  월드 위치가 PIP 프러스텀 밖이거나 캡처 카메라 뒤쪽이라 정상 투영되지 않으면, PIP 영역
 *  가장자리에서 OffscreenMargin만큼 안쪽으로 들어온 위치로 클램프해서 표시한다 - 화면 밖으로
 *  떨어진 코인이라도 안내 문구는 항상 PIP 화면 안에서 보이게 하기 위함
 */
UCLASS(abstract)
class CP_API UCPCoinPointUI : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 낱개 텍스트 위젯들이 배치될 캔버스 (선택 사항 - 없으면 아무것도 표시되지 않는다) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> PointTextCanvas;

	/** ShowPointText가 호출될 때마다 생성하는 낱개 텍스트 위젯 클래스 (없으면 아무 동작도 하지 않는다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point")
	TSubclassOf<UCPCoinPointTextWidget> PointTextWidgetClass;

	/** 텍스트가 화면에 머무는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point", meta = (ClampMin = 0))
	float DisplayDuration = 1.0f;

	/** PIP 프러스텀 밖(또는 캡처 카메라 뒤)으로 투영된 위치를 PIP 영역 안쪽으로 클램프할 때,
	 *  가장자리로부터 띄우는 여백(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point", meta = (ClampMin = 0))
	float OffscreenMargin = 32.0f;

	/** ACPDropZone::OnCoinDropped(FVector)에 그대로 Bind Event하기 위한 ShowCoinPointText가
	 *  사용하는 기본 문구 (예: "+1") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point")
	FText CoinPointDisplayText = FText::FromString(TEXT("+1"));

	/** UCPCoinPusherCaptureWidget/UCPCoinPusherViewportClient와 반드시 같은 값으로 맞춰야 하는,
	 *  PIP가 화면 왼쪽에서 차지하는 비율(0~1) - 이 값을 기준으로 캡처 카메라의 투영 결과를 화면
	 *  픽셀 좌표로 매핑한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point", meta = (ClampMin = 0, ClampMax = 1))
	float CaptureWidthRatio = 0.3f;

	/** 레벨에서 찾은 ACPCoinPusher의 캡처 컴포넌트를 최초 1회 캐싱 (GetCaptureComponent 참고) */
	mutable TWeakObjectPtr<UCPCoinPusherViewCaptureComponent> CachedCaptureComponent;

public:

	/** WorldLocation을 PIP 화면 좌표로 계산해서(프러스텀 밖/캡처 카메라 뒤쪽이면 OffscreenMargin만큼
	 *  안쪽으로 클램프) 그 자리에 Text를 보여주는 PointTextWidgetClass 인스턴스를 하나 생성하고,
	 *  DisplayDuration 후 제거한다. PointTextCanvas/PointTextWidgetClass가 없거나 레벨에서
	 *  ACPCoinPusher의 캡처 컴포넌트를 찾을 수 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void ShowPointText(const FText& Text, FVector WorldLocation);

	/** ACPDropZone::OnCoinDropped(FVector)에 그대로 Bind Event할 수 있도록, CoinPointDisplayText를
	 *  문구로 사용해 ShowPointText를 호출하는 얇은 래퍼 */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void ShowCoinPointText(FVector WorldLocation);

protected:

	/** 레벨에 배치된 ACPCoinPusher를 찾아 그 GetViewCaptureComponent()를 반환하고 캐싱한다(최초
	 *  1회만 탐색). 찾지 못하면 nullptr */
	UCPCoinPusherViewCaptureComponent* GetCaptureComponent() const;

	/** WorldLocation을 ACPCoinPusher의 캡처 카메라(GetCaptureComponent) 기준으로 투영해서, 화면
	 *  왼쪽 PIP 영역(가로 CaptureWidthRatio * 뷰포트 너비, 세로 뷰포트 전체) 안의 픽셀 좌표로
	 *  변환한다. PIP 프러스텀 안에 정상적으로 들어오면 그 좌표를, 프러스텀 밖이거나 캡처 카메라
	 *  뒤쪽이면 OffscreenMargin만큼 안쪽으로 들여온 좌표를 OutViewportPosition에 채워 반환한다.
	 *  캡처 컴포넌트를 찾을 수 없거나 PIP 영역 크기가 0이면 false 반환 */
	bool CalculateClampedScreenPosition(const FVector& WorldLocation, FVector2D& OutViewportPosition) const;

	/** DisplayDuration 후 호출되어 PointTextCanvas에서 해당 위젯을 제거하는 타이머 콜백 */
	void RemovePointText(UCPCoinPointTextWidget* PointTextWidget);
};

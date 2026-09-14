// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCoinPointUI.generated.h"

class ACPCoinPusher;
class UCPCoinPusherViewCaptureComponent;
class UCanvasPanel;
class UCPCoinPointTextWidget;

/**
 *  코인/아이템이 DropZone에 떨어진 월드 위치를 이 위젯 안 PointTextCanvas(정해진 영역)의 좌표로
 *  계산해서, 그 자리에 안내 문구를 잠깐 띄웠다가 사라지게 하는 UI. UCPInGameWidget("InGameUI")이
 *  컴포넌트로 갖고 있으며, 실제 드랍 이벤트는 ACPDropZone::OnCoinDropped(FName, FVector)
 *  델리게이트가 ACPCoinPusher::BindDropZoneEventsToInGameUI()에 의해 이 위젯의 ShowCoinPointText에
 *  C++에서 자동으로 바인딩되어 받는다 (WBP에서 별도로 Bind Event를 걸 필요 없음). 표시 문구는
 *  ACPCoinPusher::GetItemDataTable()에서 떨어진 ItemID로 FItemData 행을 찾아 그 CoinPointText를
 *  사용하고, 행을 못 찾거나 CoinPointText가 비어있으면 CoinPointDisplayText(기본 "+1")로
 *  대체한다 - 그 외 임의의 문구를 원하는 위치에 띄우고 싶으면 ShowPointText(Text, Location)를
 *  직접 호출).
 *
 *  가로(X) 위치는 ACPCoinPusher::GetViewCaptureComponent()(코인 푸셔를 비추는 Screen Capture
 *  카메라 - 플레이어를 따라다니는 Main Camera와는 별개)의 실제 카메라 투영으로 계산한다: WorldLocation을
 *  그 카메라 기준 로컬 공간으로 변환해 FOV(Perspective) 또는 OrthoWidth(Orthographic)로 좌우 NDC
 *  값을 구하고, PointTextCanvas의 가로 픽셀 폭에 매핑한다 - Screen Capture 카메라가 실제로 그 화면에
 *  담아내는 좌우 위치와 정확히 일치시키기 위함(단순 선형 Box 정규화로는 카메라가 보는 시야와
 *  어긋날 수 있어 이 방식으로 바꿈). 세로(Y) 위치는 드랍 위치와 무관하게 항상 FixedVerticalRatio
 *  비율의 고정 위치에 즉시 뜬다(깊이에 따라 위아래로 흩어지지 않음). 카메라 뒤쪽이거나 시야
 *  밖이면 PointTextCanvas 가장자리에서 OffscreenMargin만큼 안쪽으로 들어온 위치로 클램프해서
 *  표시한다.
 *
 *  PointTextCanvas(UCanvasPanel, BindWidgetOptional) 위에 PointTextWidgetClass
 *  (UCPCoinPointTextWidget 상속 WBP)의 인스턴스를 호출마다 하나씩 생성해 계산된 좌표에 배치하고,
 *  DisplayDuration 후 제거한다 - 동시에 여러 코인/아이템이 떨어져도 각자 독립된 인스턴스로 겹쳐
 *  표시된다.
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

	/** Screen Capture 카메라 시야 밖(또는 카메라 뒤쪽)으로 계산된 위치를 PointTextCanvas 안쪽으로
	 *  클램프할 때, 가장자리로부터 띄우는 여백(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point", meta = (ClampMin = 0))
	float OffscreenMargin = 32.0f;

	/** 세로(Y) 위치는 드랍된 월드 위치와 무관하게 항상 이 비율(0=위쪽 끝, 1=아래쪽 끝)의 고정
	 *  위치에 뜬다 - 가로(X)만 드랍 위치를 따라가고, 세로는 즉시 일정한 자리에 나타나게 하기 위함 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point", meta = (ClampMin = 0, ClampMax = 1))
	float FixedVerticalRatio = 0.0f;

	/** ShowCoinPointText가 ItemDataTable에서 떨어진 ItemID의 행을 못 찾거나, 그 행의
	 *  CoinPointText가 비어있을 때 대신 사용하는 기본 문구 (예: "+1") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point")
	FText CoinPointDisplayText = FText::FromString(TEXT("+1"));

	/** 레벨에서 찾은 ACPCoinPusher를 최초 1회 캐싱 (GetCoinPusher 참고) - Screen Capture 카메라와
	 *  ItemDataTable 조회가 함께 공유해서 쓴다 */
	mutable TWeakObjectPtr<ACPCoinPusher> CachedCoinPusher;

	/** 레벨에서 찾은 ACPCoinPusher의 Screen Capture 카메라(ViewCaptureComponent)를 최초 1회
	 *  캐싱 (GetCaptureComponent 참고) */
	mutable TWeakObjectPtr<UCPCoinPusherViewCaptureComponent> CachedCaptureComponent;

public:

	/** WorldLocation을 PointTextCanvas 좌표로 계산해서(Screen Capture 카메라 시야 밖이면
	 *  OffscreenMargin만큼 안쪽으로 클램프) 그 자리에 Text를 보여주는 PointTextWidgetClass 인스턴스를
	 *  하나 생성하고, DisplayDuration 후 제거한다. PointTextCanvas/PointTextWidgetClass가 없거나
	 *  레벨에서 ACPCoinPusher의 Screen Capture 카메라를 찾을 수 없으면 아무 동작도 하지 않는다.
	 *  좌표 계산에 성공하면 WorldLocation/계산된 ViewportPosition을 LogUI로 남긴다(디버깅용) */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void ShowPointText(const FText& Text, FVector WorldLocation);

	/** ACPDropZone::OnCoinDropped(FName, FVector)와 시그니처가 같아
	 *  ACPCoinPusher::BindDropZoneEventsToInGameUI()이 자동으로 바인딩하는 대상 - ItemID로
	 *  ACPCoinPusher::GetItemDataTable()에서 FItemData 행을 찾아 그 CoinPointText를 문구로
	 *  ShowPointText를 호출한다(행을 못 찾거나 CoinPointText가 비어있으면 CoinPointDisplayText 사용) */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void ShowCoinPointText(FName ItemID, FVector WorldLocation);

protected:

	/** 레벨에 배치된 ACPCoinPusher를 찾아 캐싱한다(최초 1회만 탐색). 찾지 못하면 nullptr */
	ACPCoinPusher* GetCoinPusher() const;

	/** GetCoinPusher()의 GetViewCaptureComponent()(Screen Capture 카메라)를 반환하고 캐싱한다
	 *  (최초 1회만 탐색). 찾지 못하면 nullptr */
	UCPCoinPusherViewCaptureComponent* GetCaptureComponent() const;

	/** GetCoinPusher()의 GetItemDataTable()에서 ItemID로 FItemData 행을 찾아 그 CoinPointText를
	 *  반환한다. 행을 못 찾거나 CoinPointText가 비어있으면(또는 ItemDataTable/CoinPusher가 없으면)
	 *  CoinPointDisplayText를 대신 반환한다 */
	FText ResolveCoinPointText(FName ItemID) const;

	/** WorldLocation을 ACPCoinPusher의 Screen Capture 카메라(GetCaptureComponent) 기준으로 투영해
	 *  가로(X) NDC 값을 구하고, PointTextCanvas(정해진 영역)의 가로 픽셀 폭에 매핑한다(카메라 시야
	 *  밖/뒤쪽이면 OffscreenMargin만큼 안쪽으로 클램프). 세로(Y) 위치는 드랍 위치와 무관하게 항상
	 *  FixedVerticalRatio 비율의 고정 위치로 채워진다. OutViewportPosition에 결과를 채워 반환하며,
	 *  Screen Capture 카메라를 찾을 수 없거나 PointTextCanvas 크기가 0이면 false 반환 */
	bool CalculateClampedScreenPosition(const FVector& WorldLocation, FVector2D& OutViewportPosition) const;

	/** DisplayDuration 후 호출되어 PointTextCanvas에서 해당 위젯을 제거하는 타이머 콜백 */
	void RemovePointText(UCPCoinPointTextWidget* PointTextWidget);
};

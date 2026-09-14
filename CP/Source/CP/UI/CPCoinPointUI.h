// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCoinPointUI.generated.h"

class ACPCoinPusher;
class ACPDropZone;
class UCanvasPanel;
class UCPCoinPointTextWidget;

/**
 *  코인/아이템이 DropZone에 떨어진 월드 위치를 이 위젯 안 PointTextCanvas(정해진 영역)의 좌표로
 *  계산해서, 그 자리에 안내 문구(예: "+1")를 잠깐 띄웠다가 사라지게 하는 UI. UCPInGameWidget
 *  ("InGameUI")이 컴포넌트로 갖고 있으며, 실제 드랍 이벤트는 ACPDropZone::OnCoinDropped(FVector)
 *  델리게이트가 ACPCoinPusher::BindDropZoneEventsToInGameUI()에 의해 이 위젯의 ShowCoinPointText에
 *  C++에서 자동으로 바인딩되어 받는다 (WBP에서 별도로 Bind Event를 걸 필요 없음 - 그 외 임의의
 *  문구를 원하는 위치에 띄우고 싶으면 ShowPointText(Text, Location)를 직접 호출).
 *
 *  카메라 투영을 쓰지 않는 단순한 선형 매핑이다: 레벨에서 찾은 ACPCoinPusher::GetDropZone()의
 *  CollectionVolume(UBoxComponent) Extent를 기준으로, WorldLocation의 DropZone 로컬 오프셋을
 *  Z값은 PointTextCanvas의 가로(X) 위치로, X값은 PointTextCanvas의 세로(Y) 위치로 정규화해
 *  매핑한다 (DropZone의 물리적 배치와 UI 표시 영역의 가로/세로가 다르게 대응되는 것은 의도된
 *  디자인). DropZone의 Box 범위를 벗어나는 값은 OffscreenMargin만큼 안쪽으로 들어온 위치로
 *  클램프해서 표시한다 - Box 밖에서 판정된 위치라도 안내 문구는 항상 PointTextCanvas 안에서
 *  보이게 하기 위함.
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

	/** DropZone의 Box 범위 밖으로 정규화된 위치를 PointTextCanvas 안쪽으로 클램프할 때, 가장자리로부터
	 *  띄우는 여백(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point", meta = (ClampMin = 0))
	float OffscreenMargin = 32.0f;

	/** ACPDropZone::OnCoinDropped(FVector)에 자동으로 바인딩되는 ShowCoinPointText가 사용하는
	 *  기본 문구 (예: "+1") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Point")
	FText CoinPointDisplayText = FText::FromString(TEXT("+1"));

	/** 레벨에서 찾은 ACPCoinPusher의 DropZone을 최초 1회 캐싱 (GetDropZone 참고) */
	mutable TWeakObjectPtr<ACPDropZone> CachedDropZone;

public:

	/** WorldLocation을 PointTextCanvas 좌표로 계산해서(DropZone Box 범위 밖이면 OffscreenMargin만큼
	 *  안쪽으로 클램프) 그 자리에 Text를 보여주는 PointTextWidgetClass 인스턴스를 하나 생성하고,
	 *  DisplayDuration 후 제거한다. PointTextCanvas/PointTextWidgetClass가 없거나 레벨에서
	 *  ACPCoinPusher의 DropZone을 찾을 수 없으면 아무 동작도 하지 않는다. 좌표 계산에 성공하면
	 *  WorldLocation/계산된 ViewportPosition을 LogUI로 남긴다(디버깅용) */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void ShowPointText(const FText& Text, FVector WorldLocation);

	/** ACPDropZone::OnCoinDropped(FVector)와 시그니처가 같아 ACPCoinPusher::BindDropZoneEventsToInGameUI()이
	 *  자동으로 바인딩하는 대상 - CoinPointDisplayText를 문구로 사용해 ShowPointText를 호출하는 얇은 래퍼 */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void ShowCoinPointText(FVector WorldLocation);

protected:

	/** 레벨에 배치된 ACPCoinPusher를 찾아 그 GetDropZone()을 반환하고 캐싱한다(최초 1회만 탐색).
	 *  찾지 못하면 nullptr */
	ACPDropZone* GetDropZone() const;

	/** WorldLocation을 ACPDropZone::GetCollectionVolume()의 Box Extent 기준으로 정규화해,
	 *  PointTextCanvas(정해진 영역)의 로컬 픽셀 좌표로 변환한다 - WorldLocation의 DropZone 로컬
	 *  오프셋 중 Z값을 PointTextCanvas의 가로(X) 위치로, X값을 세로(Y) 위치로 매핑한다(카메라 투영
	 *  없는 단순 선형 매핑). Box 범위를 벗어나면 OffscreenMargin만큼 안쪽으로 들여온 좌표로 클램프해
	 *  OutViewportPosition에 채워 반환한다. DropZone을 찾을 수 없거나 PointTextCanvas 크기가 0이면
	 *  false 반환 */
	bool CalculateClampedScreenPosition(const FVector& WorldLocation, FVector2D& OutViewportPosition) const;

	/** DisplayDuration 후 호출되어 PointTextCanvas에서 해당 위젯을 제거하는 타이머 콜백 */
	void RemovePointText(UCPCoinPointTextWidget* PointTextWidget);
};

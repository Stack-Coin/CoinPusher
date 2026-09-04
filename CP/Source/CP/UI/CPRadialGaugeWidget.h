// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPRadialGaugeWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;

/**
 *  진짜 원(파이 조각)이 채워지는 것처럼 보이는 원형 게이지 위젯의 베이스 클래스.
 *  UMG의 ProgressBar/Image는 좌우/상하 방향 채우기만 지원하고 각도 기반(radial) 채우기는
 *  지원하지 않기 때문에, FillImage에는 각도 기반 마스크를 구현하는 Material을 브러시로 지정하고
 *  이 클래스가 그 Material의 Scalar Parameter(PercentParameterName, 기본 "Percent")를
 *  Current/Max 비율로 갱신해주는 방식으로 동작한다.
 *
 *  배경(BackgroundImage) 위에 FillImage가 겹쳐 있는 구조는 체력바(UCPHealthBarWidget)와 동일.
 *  NativeConstruct에서 FillImage에 지정된 Material로부터 Dynamic Material Instance를 하나 만들어
 *  캐싱해두고, 이후 SetGaugePercent가 호출될 때마다 그 인스턴스의 Percent 파라미터만 갱신한다.
 *
 *  WBP에서 준비해야 하는 것: FillImage의 Brush(Image)에 각도 기반 마스크를 만드는 Material을
 *  지정해야 한다(하위 README 참고 - Radial Gradient Exponential 노드 등으로 만들 수 있음).
 *  Material 없이 그냥 텍스처만 지정하면 파라미터를 갱신할 대상이 없어 아무 효과도 나지 않는다.
 */
UCLASS(abstract)
class CP_API UCPRadialGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 게이지 틀 역할을 하는 배경 원 이미지 (선택 사항 - 없어도 동작에는 지장 없음) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	/** 실제 값을 나타내는 원 이미지. Brush에 각도 기반 마스크 Material이 지정돼 있어야
	 *  파이 조각이 채워지는 것처럼 보인다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> FillImage;

	/** FillImage에 지정한 Material의 Scalar Parameter 중, 채워진 비율(0~1)을 나타내는
	 *  파라미터 이름. Material 쪽 파라미터 이름과 반드시 일치해야 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gauge")
	FName PercentParameterName = TEXT("Percent");

	/** FillImage의 Material로부터 만든 Dynamic Material Instance. NativeConstruct에서 한 번만
	 *  생성해서 캐싱해두고, 이후로는 파라미터 값만 갱신한다 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FillMaterialInstance;

	/** FillImage의 Brush에 지정된 Material로부터 FillMaterialInstance를 만든다 */
	virtual void NativeConstruct() override;

public:

	/** 값 변경 델리게이트에 바인딩해서 쓰는 진입점. 0-1 Percent를 계산해 SetGaugePercent를 호출 */
	UFUNCTION(BlueprintCallable, Category="Gauge")
	void UpdateGauge(float CurrentValue, float MaxValue);

	/** 게이지를 0-1 Percent에 맞춰 갱신한다. 기본 구현은 FillMaterialInstance의
	 *  PercentParameterName 스칼라 파라미터를 Percent로 설정한다 - WBP에서 오버라이드해
	 *  커스텀 연출(색상 변화 등)을 추가할 수 있다 */
	UFUNCTION(BlueprintNativeEvent, Category="Gauge")
	void SetGaugePercent(float Percent);
	virtual void SetGaugePercent_Implementation(float Percent);
};

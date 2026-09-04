// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "CPRadialGaugeComponent.generated.h"

class UCPRadialGaugeWidget;

/**
 *  UCPRadialGaugeWidget을 Level에 배치된 Actor 위(월드 스페이스)에 띄우는 컴포넌트.
 *  UCPHealthBarComponent와 동일한 구조 - 아무 Actor의 BP에나 Add Component로 붙이면 되고,
 *  GaugeWidgetClass에 UCPRadialGaugeWidget을 상속하는 WBP를 지정한다.
 *  체력바와 다른 점은 SetGaugeEnabled로 게이지 자체를 켜고 끌 수 있다는 것 - 항상 보일 필요가
 *  없는 게이지(예: 특정 상황에서만 나타나는 충전 게이지)에 사용
 */
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class CP_API UCPRadialGaugeComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UCPRadialGaugeComponent();

protected:

	/** 이 컴포넌트가 표시할 Widget Blueprint 클래스. 상속받은 UWidgetComponent::WidgetClass는
	 *  아무 UUserWidget이나 고를 수 있어서 실수로 안 맞는 위젯을 넣기 쉬운데, 이 프로퍼티는
	 *  UCPRadialGaugeWidget을 상속하는 클래스만 고를 수 있도록 BP Details의 클래스 피커를
	 *  좁혀준다. BeginPlay에서 SetWidgetClass(GaugeWidgetClass)로 그대로 반영됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gauge")
	TSubclassOf<UCPRadialGaugeWidget> GaugeWidgetClass;

	virtual void BeginPlay() override;

	/** Cached UserWidgetObject, cast once in BeginPlay. BlueprintReadWrite라 BP 그래프에서 직접
	 *  읽거나(커스텀 로직에 위젯 인스턴스 필요할 때) 다른 위젯 인스턴스로 바꿔 끼울 수 있다 */
	UPROPERTY(BlueprintReadWrite, Category="Gauge")
	TObjectPtr<UCPRadialGaugeWidget> GaugeWidget;

public:

	/** 값이 바뀔 때마다 호출해서 게이지에 반영한다. 대상의 값 변경 델리게이트를 BP에서
	 *  Bind Event로 이 함수에 연결해두면 자동으로 갱신된다 */
	UFUNCTION(BlueprintCallable, Category="Gauge")
	void UpdateGauge(float CurrentValue, float MaxValue);

	/** 게이지를 켜고 끈다 - Off일 때는 렌더링(가시성)과 갱신(컴포넌트 Tick) 모두 멈춘다 */
	UFUNCTION(BlueprintCallable, Category="Gauge")
	void SetGaugeEnabled(bool bEnabled);
};

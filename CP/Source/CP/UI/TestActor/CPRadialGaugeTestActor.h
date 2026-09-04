// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "CPRadialGaugeTestActor.generated.h"

class UCPRadialGaugeComponent;

/**
 *  UCPRadialGaugeComponent 동작을 확인하기 위한 테스트용 Actor.
 *  Level에 배치하고 재생하면 게이지 값이 자동으로 차올랐다가 다시 0으로 돌아가는 걸 반복하고,
 *  A 키를 누를 때마다 게이지 자체를 껐다 켰다(SetGaugeEnabled) 할 수 있다.
 *  BP 서브클래스 없이 이 클래스 자체로 바로 배치해도 되고, 이 클래스를 상속하는 BP를 만들어도 된다.
 *  Widget Blueprint 지정은 RadialGauge 컴포넌트 자신의 GaugeWidgetClass 프로퍼티(Components 탭에서
 *  RadialGauge 선택 후 편집)를 사용한다.
 *
 *  A 키 입력은 별도 Input Action/Mapping Context 에셋 없이 바로 테스트할 수 있도록 레거시
 *  raw key 바인딩(EnableInput + InputComponent->BindKey)을 사용한다 - 테스트 전용이며, 실제
 *  게임플레이 입력은 프로젝트 관례대로 Enhanced Input을 사용해야 한다.
 */
UCLASS()
class CP_API ACPRadialGaugeTestActor : public AActor
{
	GENERATED_BODY()

public:

	ACPRadialGaugeTestActor();

protected:

	/** Level에 월드 스페이스로 표시되는 원형 게이지 컴포넌트. EditAnywhere라 이 액터를 상속하는
	 *  BP의 Components 탭에서 선택해 Widget Class/Draw Size/상대 위치 등을 직접 편집할 수 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	TObjectPtr<UCPRadialGaugeComponent> RadialGauge;

	/** 테스트용 최대값 */
	UPROPERTY(EditAnywhere, Category="Gauge Test", meta = (ClampMin = 1))
	float MaxValue = 100.0f;

	/** 테스트용 현재값 (에디터에서 직접 바꿔가며 확인할 수도 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gauge Test")
	float CurrentValue = 0.0f;

	/** ChangeInterval마다 채울 값 */
	UPROPERTY(EditAnywhere, Category="Gauge Test", meta = (ClampMin = 0))
	float ChangeAmount = 10.0f;

	/** 자동 변경 틱 간격(초). 0 이하로 두면 자동 변경 없이 CurrentValue를 직접 바꾸거나
	 *  RadialGauge->UpdateGauge를 수동으로 호출해서 테스트 가능 */
	UPROPERTY(EditAnywhere, Category="Gauge Test", meta = (ClampMin = 0))
	float ChangeInterval = 0.5f;

	/** 현재 게이지가 켜져 있는 상태인지 - A 키를 누를 때마다 토글됨 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gauge Test")
	bool bGaugeEnabled = true;

	FTimerHandle ValueTimerHandle;

	virtual void BeginPlay() override;

	/** ChangeInterval마다 호출 - CurrentValue를 ChangeAmount만큼 채우고, MaxValue를 넘으면 다시
	 *  0부터 채우기 시작한다. 매번 RadialGauge->UpdateGauge를 호출해 반영 */
	void HandleValueTick();

public:

	/** RadialGauge를 켜고 끈다 (A 키에 바인딩됨) */
	UFUNCTION(BlueprintCallable, Category="Gauge Test")
	void ToggleGauge();
};

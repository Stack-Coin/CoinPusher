// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPPressAnyKeyWidget.generated.h"

class IInputProcessor;

/** 키보드/마우스/게임패드 어떤 입력이든 눌렸을 때 브로드캐스트되는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCPAnyKeyPressed, FKey, PressedKey);

/**
 *  "아무 버튼이나 누르면 다음 화면으로 넘어가는" 타이틀/안내 화면 등에 쓰는 범용 UI 클래스.
 *  포커스나 히트테스트에 의존하지 않고, 전역 입력 프로세서(FCPAnyInputProcessor)로 키보드/마우스/
 *  게임패드 입력을 직접 가로채서 OnAnyKeyPressed를 Broadcast한다 - 이 위젯이 화면 어디를
 *  덮고 있는지, 어떤 위젯이 키보드 포커스를 가졌는지와 무관하게 항상 동작한다.
 *  NextWidgetClass를 지정해두면 별도 BP 그래프 작업 없이도 자동으로 그 위젯으로 전환된다.
 *  더 복잡한 전환 로직이 필요하면 OnAnyKeyPressed 델리게이트를 직접 바인딩해서 처리하면 된다.
 */
UCLASS(abstract)
class CP_API UCPPressAnyKeyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 지정해두면 아무 입력이나 눌렸을 때 자동으로 이 위젯을 생성해 화면에 띄운다 (선택 사항) */
	UPROPERTY(EditAnywhere, Category="UI Switch")
	TSubclassOf<UUserWidget> NextWidgetClass;

	/** NextWidgetClass로 전환할 때 이 위젯 자신을 화면에서 제거할지 여부 */
	UPROPERTY(EditAnywhere, Category="UI Switch")
	bool bRemoveSelfOnSwitch = true;

	/** 첫 입력이 감지된 후 NextWidgetClass로 전환하기까지 대기하는 시간(초). 0이면(기본값) 즉시
	 *  전환한다 - 연출을 위해 잠깐 대기해야 하는 화면(예: 타이틀 화면)에서만 늘려주면 된다 */
	UPROPERTY(EditAnywhere, Category="UI Switch", meta = (ClampMin = 0))
	float SwitchDelay = 0.0f;

	/** SwitchDelay 대기 후 실제 전환(SwitchToNextWidget)을 실행하는 타이머 핸들 */
	FTimerHandle SwitchTimerHandle;

	/** 이 위젯이 살아있는 동안 등록해두는 전역 입력 프로세서. NativeDestruct에서 해제한다 */
	TSharedPtr<IInputProcessor> AnyKeyInputProcessor;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 입력이 감지됐을 때 공통으로 처리하는 내부 함수 - 델리게이트 Broadcast 후
	 *  ScheduleSwitchToNextWidget()을 호출 */
	void HandleAnyKeyPressed(FKey PressedKey);

	/** NextWidgetClass가 설정되어 있으면 SwitchDelay만큼 기다렸다가(0이면 즉시) SwitchToNextWidget()을
	 *  실행되도록 예약한다. 하위 클래스가 전환 타이밍을 직접 제어하고 싶으면(예: 전환 전에 연출을
	 *  재생한 뒤 자기 타이밍에 SwitchToNextWidget()을 호출) 이 함수를 오버라이드하면 된다 */
	virtual void ScheduleSwitchToNextWidget();

	/** NextWidgetClass가 설정되어 있으면 그 위젯을 생성해 화면에 띄우고, bRemoveSelfOnSwitch면
	 *  이 위젯 자신을 제거한다. 하위 클래스가 자신만의 조건/딜레이로 직접 전환을 실행하고 싶을 때도
	 *  재사용할 수 있도록 protected로 둠 */
	void SwitchToNextWidget();

public:

	/** 아무 입력이나 눌렸을 때 Broadcast. BP에서 자유롭게 바인딩해서 커스텀 전환 로직을 붙일 수 있다 */
	UPROPERTY(BlueprintAssignable, Category="UI Switch")
	FCPAnyKeyPressed OnAnyKeyPressed;
};

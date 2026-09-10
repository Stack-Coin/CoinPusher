// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CPPressAnyKeyWidget.h"
#include "GameMode/CPControllerType.h"
#include "CPStartScreenWidget.generated.h"

class UImage;

/**
 *  게임 시작 시 처음 뜨는 화면. BackGround/GamePad/KeyBoard 이미지와 "Press Any Key" 안내 텍스트로
 *  구성된다(안내 텍스트는 WBP에 고정 배치, 특별한 C++ 바인딩 불필요). UCPPressAnyKeyWidget을 상속해
 *  아무 입력이나 눌리면(OnAnyKeyPressed) 게임패드 입력이면 KeyBoardImage를, 키보드/마우스 입력이면
 *  GamePadImage를 숨기고 반대쪽(방금 사용한 장치의 이미지)은 다시 보이게 해서, 마지막으로 사용한
 *  장치 하나만 화면에 남도록 한다. 그렇게 감지된 컨트롤러 종류는 첫 입력 때 UCPControllerTypeSubsystem
 *  에 기록되어, 다음 화면(UCPGameExplanationWidget)이 레벨을 넘어가지 않고도 곧바로 참조할 수 있다.
 *  이후 선택된(=화면에 남은) 아이콘을 BlinkCount번 BlinkInterval초 간격으로 점멸시킨 뒤에야 다음
 *  화면으로 전환한다 - 부모 클래스의 즉시/고정 딜레이 전환 대신 ScheduleSwitchToNextWidget()을
 *  오버라이드해 점멸 시퀀스가 끝난 뒤 SwitchToNextWidget()을 직접 호출하는 방식
 */
UCLASS(abstract)
class CP_API UCPStartScreenWidget : public UCPPressAnyKeyWidget
{
	GENERATED_BODY()

protected:

	/** 게임패드 아이콘. 키보드/마우스 입력이 감지되면 숨겨진다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> GamePadImage;

	/** 키보드 아이콘. 게임패드 입력이 감지되면 숨겨진다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> KeyBoardImage;

	/** 컨트롤러가 선택된 뒤 해당 아이콘을 몇 번 점멸(껐다 켜는 것 한 번 기준)시킬지 */
	UPROPERTY(EditAnywhere, Category="Start Screen", meta = (ClampMin = 1))
	int32 BlinkCount = 3;

	/** 점멸 한 단계(켜짐↔꺼짐 전환)마다 대기하는 시간(초) */
	UPROPERTY(EditAnywhere, Category="Start Screen", meta = (ClampMin = 0.01))
	float BlinkInterval = 0.5f;

	/** 점멸 시퀀스가 이미 시작됐으면 true - 이후 입력은 무시해 선택을 다시 바꾸지 않는다 */
	bool bHasStartedBlink = false;

	/** 현재 점멸 중인 아이콘 (선택된 컨트롤러 쪽) */
	TObjectPtr<UImage> BlinkingImage;

	/** 점멸 시퀀스에서 지금까지 토글된 횟수 (BlinkCount * 2에 도달하면 종료) */
	int32 BlinkToggleCount = 0;

	/** BlinkInterval마다 BlinkingImage의 가시성을 토글하는 타이머 핸들 */
	FTimerHandle BlinkTimerHandle;

	virtual void NativeConstruct() override;

	/** OnAnyKeyPressed에 바인딩 - 점멸이 아직 시작되지 않았을 때만, 입력 장치 종류에 따라 반대쪽
	 *  아이콘을 숨기고 자신의 아이콘은 보이게 한 뒤, 그 결과를 UCPControllerTypeSubsystem에 기록한다 */
	UFUNCTION()
	void HandleControllerInputDetected(FKey PressedKey);

	/** 부모의 즉시/고정 딜레이 전환을 대체 - 화면에 남아있는(=선택된) 아이콘을 찾아 점멸 시퀀스를
	 *  시작한다. 이미 시작됐거나 NextWidgetClass가 없으면(또는 점멸시킬 아이콘이 없으면) 부모와
	 *  동일하게 처리하거나 그냥 무시한다 */
	virtual void ScheduleSwitchToNextWidget() override;

	/** BlinkTimerHandle에 의해 BlinkInterval마다 반복 호출 - BlinkingImage의 가시성을 토글하고,
	 *  BlinkCount * 2번 토글하면(꺼짐/켜짐 합쳐서 BlinkCount번 점멸) 타이머를 멈추고
	 *  SwitchToNextWidget()으로 다음 화면으로 전환한다 */
	void HandleBlinkTick();
};

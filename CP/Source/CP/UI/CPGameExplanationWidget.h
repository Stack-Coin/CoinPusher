// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CPPressAnyKeyWidget.h"
#include "GameMode/CPControllerType.h"
#include "CPGameExplanationWidget.generated.h"

class UImage;

/**
 *  시작 화면(UCPStartScreenWidget) 다음에 뜨는 게임 설명 화면. UCPControllerTypeSubsystem에 기록된
 *  선택 결과에 따라 GamePadImage 또는 KeyBoardImage 중 하나만 보여주고("Press Any Key to Start"
 *  안내 텍스트는 WBP에 고정 배치), 그 선택된 컨트롤러 종류와 일치하는 입력이 들어왔을 때만
 *  (부모의) NextWidgetClass로 지정된 컷신 위젯(UCPCutsceneWidget)으로 전환한다 - 선택하지 않은
 *  쪽 장치로 누르면 무시된다. 부모의 ScheduleSwitchToNextWidget()을 빈 오버라이드로 막아뒀는데,
 *  안 막으면 컨트롤러 종류를 가리지 않고 아무 입력에나 자동 전환되어(부모 HandleAnyKeyPressed가
 *  매 입력마다 호출) 이 위젯의 "일치하는 입력만 처리" 규칙이 깨지기 때문이다 - 전환은 항상
 *  HandleSelectedControllerKeyPressed에서 매칭을 통과했을 때만 SwitchToNextWidget()으로 직접
 *  실행한다.
 */
UCLASS(abstract)
class CP_API UCPGameExplanationWidget : public UCPPressAnyKeyWidget
{
	GENERATED_BODY()

protected:

	/** 시작 화면에서 게임패드가 선택된 경우에만 보임 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> GamePadImage;

	/** 시작 화면에서 키보드/마우스가 선택된 경우에만 보임 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> KeyBoardImage;

	/** 전환을 이미 요청했으면 true - 중복 입력으로 두 번 전환되는 것을 막는다 */
	bool bHasRequestedLevelChange = false;

	virtual void NativeConstruct() override;

	/** 부모의 자동 전환(모든 입력에 반응)을 막기 위한 빈 오버라이드 - 실제 전환은
	 *  HandleSelectedControllerKeyPressed가 매칭을 통과했을 때 SwitchToNextWidget()을 직접 호출 */
	virtual void ScheduleSwitchToNextWidget() override;

	/** OnAnyKeyPressed에 바인딩 - 시작 화면에서 선택된 컨트롤러와 종류가 일치하는 입력만 처리해
	 *  NextWidgetClass(컷신 위젯)로 전환한다 (일치하지 않으면 무시) */
	UFUNCTION()
	void HandleSelectedControllerKeyPressed(FKey PressedKey);
};

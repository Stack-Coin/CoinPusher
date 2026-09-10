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
 *  NextLevelName으로 레벨을 전환한다 - 선택하지 않은 쪽 장치로 누르면 무시된다. 선택된 컨트롤러
 *  정보는 UCPControllerTypeSubsystem이 GameInstance에 붙어 있어 OpenLevel 이후에도 그대로
 *  유지되므로, 다음 레벨에서도 별도 파라미터 없이 그대로 조회할 수 있다.
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

	/** 선택된 컨트롤러로 아무 입력이나 누르면 이동할 다음 레벨 */
	UPROPERTY(EditAnywhere, Category="Game Explanation")
	FName NextLevelName;

	/** OpenLevel을 이미 호출했으면 true - 전환 중 중복 입력으로 두 번 호출되는 것을 막는다 */
	bool bHasRequestedLevelChange = false;

	virtual void NativeConstruct() override;

	/** OnAnyKeyPressed에 바인딩 - 시작 화면에서 선택된 컨트롤러와 종류가 일치하는 입력만 처리해
	 *  NextLevelName으로 이동한다 (일치하지 않으면 무시) */
	UFUNCTION()
	void HandleSelectedControllerKeyPressed(FKey PressedKey);
};

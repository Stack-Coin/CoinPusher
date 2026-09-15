// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CPPressAnyKeyWidget.h"
#include "CPCutsceneWidget.generated.h"

/**
 *  게임 설명 화면(UCPGameExplanationWidget) 다음, 인게임 레벨 진입 전에 뜨는 컷신 화면.
 *  UCPPressAnyKeyWidget을 그대로 상속해 아무 입력이나 감지되면(컨트롤러 종류 구분 없이) 바로
 *  NextLevelName으로 레벨을 전환한다 - 즉 컷신은 언제든 스킵 가능하다.
 */
UCLASS(abstract)
class CP_API UCPCutsceneWidget : public UCPPressAnyKeyWidget
{
	GENERATED_BODY()

protected:

	/** 아무 입력이나 눌렸을 때(=스킵) 이동할 다음 레벨 */
	UPROPERTY(EditAnywhere, Category="Cutscene")
	FName NextLevelName;

	/** OpenLevel을 이미 호출했으면 true - 중복 입력으로 두 번 호출되는 것을 막는다 */
	bool bHasRequestedLevelChange = false;

	virtual void NativeConstruct() override;

	/** OnAnyKeyPressed에 바인딩 - 아무 입력이나 감지되면 NextLevelName으로 이동 */
	UFUNCTION()
	void HandleAnyKeyPressedForSkip(FKey PressedKey);
};

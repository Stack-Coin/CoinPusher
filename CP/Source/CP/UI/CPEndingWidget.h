// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CPInGamePauseWidget.h"
#include "CPEndingWidget.generated.h"

class UImage;

/**
 *  특정 조건(레벨 클리어/게임 오버 등)이 만족됐을 때 게임을 일시정지하고 띄우는 엔딩 화면.
 *  UCPInGamePauseWidget을 상속해 반투명 배경/게임 종료/타이틀로 돌아가기 버튼과 그 선택·실행
 *  로직(마우스 클릭, 게임패드 L-Stick+A버튼)을 그대로 재사용하고, 여기에 결과 이미지(Clear/Lose)
 *  전환만 추가한다.
 */
UCLASS(abstract)
class CP_API UCPEndingWidget : public UCPInGamePauseWidget
{
	GENERATED_BODY()

protected:

	/** ShowResult(true)일 때 보이는 클리어 이미지 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ClearImage;

	/** ShowResult(false)일 때 보이는 패배(Lose) 이미지 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> LoseImage;

public:

	/** bIsClear에 따라 ClearImage/LoseImage 중 하나만 보이도록 전환 - ACPTopDownPlayerController가
	 *  이 위젯을 화면에 띄우기 직전(RefreshForDisplay와 함께) 호출한다 */
	UFUNCTION(BlueprintCallable, Category="Ending")
	void ShowResult(bool bIsClear);
};

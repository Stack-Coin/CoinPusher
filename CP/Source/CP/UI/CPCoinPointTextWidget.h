// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCoinPointTextWidget.generated.h"

class UTextBlock;

/**
 *  UCPCoinPointUI가 코인이 떨어진 위치마다 하나씩 생성해서 잠깐 띄우는 낱개 텍스트 위젯.
 *  PointText(TextBlock, BindWidgetOptional)에 문구를 표시하는 것 말고는 아무 로직도 갖지 않는다.
 *  화면에서 사라지는 타이밍은 UCPCoinPointUI가 타이머로 관리하므로(이 위젯은 자기 수명을 스스로
 *  결정하지 않음), 등장 연출(페이드 인, 위로 떠오르는 움직임 등)이 필요하면 PlayAppearEffect를
 *  WBP에서 오버라이드해(BlueprintImplementableEvent) UMG Animation 등을 재생하면 된다
 */
UCLASS(abstract)
class CP_API UCPCoinPointTextWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 표시할 문구가 들어갈 TextBlock (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PointText;

public:

	/** 표시할 문구를 설정 - PointText가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Coin Point")
	void SetPointText(const FText& InText);

	/** SetPointText 직후 호출됨 - 기본 구현은 아무 것도 하지 않으며, WBP에서 오버라이드해
	 *  등장 연출(페이드 인, 위로 떠오르는 UMG Animation 등)을 재생할 수 있다 */
	UFUNCTION(BlueprintImplementableEvent, Category="Coin Point")
	void PlayAppearEffect();
};

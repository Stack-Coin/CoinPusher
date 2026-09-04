// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPTimeDisplayWidget.generated.h"

class UTextBlock;

/**
 *  초 단위 값을 "MM:SS" 형태로 표시하는 UI 클래스 (제한 시간, 경과 시간 등).
 *  시간 변경 델리게이트를 UpdateTime에 바인딩해두면, Broadcast될 때마다 자동으로 텍스트가 갱신된다.
 */
UCLASS(abstract)
class CP_API UCPTimeDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 시간을 표시할 TextBlock. 없어도 동작은 하지만(텍스트 갱신만 생략) BP에서 배치를 권장 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeText;

public:

	/** 시간 변경 델리게이트에 바인딩해서 쓰는 진입점. TimeInSeconds를 "MM:SS"로 포맷해서 표시 */
	UFUNCTION(BlueprintCallable, Category="Time")
	void UpdateTime(float TimeInSeconds);
};

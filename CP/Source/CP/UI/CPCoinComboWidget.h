// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPCoinComboWidget.generated.h"

class UCPHorizonGuageBarWidget;
class UTextBlock;

/**
 *  코인 콤보 수(ComboCountText)와, 콤보 상태(예: 콤보가 끊기기까지 남은 시간)를 나타내는
 *  게이지 바(ComboGaugeWidget, UCPHorizonGuageBarWidget)를 함께 보여주는 UI. 둘 다
 *  BindWidgetOptional이라 배치하지 않은 쪽은 해당 값을 설정해도 조용히 무시된다.
 */
UCLASS(abstract)
class CP_API UCPCoinComboWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 콤보 수를 표시할 TextBlock (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ComboCountText;

	/** 콤보 상태(유지 시간 등)를 나타내는 게이지 바 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCPHorizonGuageBarWidget> ComboGaugeWidget;

	/** ComboCountText에 적용할 표시 형식. {0} 자리에 콤보 수가 들어간다.
	 *  기본값은 자리표시자일 뿐 - 실제 문구는 이 클래스를 상속하는 Widget Blueprint의
	 *  Class Defaults에서 지정한다 (소스 코드에 한글 리터럴을 직접 넣으면 컴파일러 소스
	 *  인코딩에 따라 깨질 수 있어 피한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin Combo")
	FText DisplayFormat = FText::FromString(TEXT("Combo x{0}"));

public:

	/** 콤보 수가 바뀔 때 호출 - ComboCountText가 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Coin Combo")
	void SetComboCount(int32 Count);

	/** 콤보 게이지 값이 바뀔 때 호출 - ComboGaugeWidget이 없으면 아무 동작도 하지 않는다 */
	UFUNCTION(BlueprintCallable, Category="Coin Combo")
	void UpdateComboGauge(float CurrentValue, float MaxValue);
};

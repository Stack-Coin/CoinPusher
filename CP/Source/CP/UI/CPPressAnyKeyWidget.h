// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPPressAnyKeyWidget.generated.h"

/** 키보드/마우스/게임패드 어떤 입력이든 눌렸을 때 브로드캐스트되는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCPAnyKeyPressed, FKey, PressedKey);

/**
 *  "아무 버튼이나 누르면 다음 화면으로 넘어가는" 타이틀/안내 화면 등에 쓰는 범용 UI 클래스.
 *  포커스를 가진 동안 키보드/마우스/게임패드 입력을 감지해 OnAnyKeyPressed를 Broadcast한다.
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

	virtual void NativeConstruct() override;

	// TODO: FReply 사용 시 "unresolved external symbol FReply::FReply(bool)" 링크 에러 발생
	// (CP.Build.cs에 SlateCore 모듈 의존성 누락이 원인) - 해결 전까지 주석 처리
	//virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	//virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 입력이 감지됐을 때 공통으로 처리하는 내부 함수 - 델리게이트 Broadcast + (설정된 경우) 위젯 전환 */
	void HandleAnyKeyPressed(FKey PressedKey);

public:

	/** 아무 입력이나 눌렸을 때 Broadcast. BP에서 자유롭게 바인딩해서 커스텀 전환 로직을 붙일 수 있다 */
	UPROPERTY(BlueprintAssignable, Category="UI Switch")
	FCPAnyKeyPressed OnAnyKeyPressed;
};

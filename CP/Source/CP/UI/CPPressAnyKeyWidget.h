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

	/** 이 위젯이 살아있는 동안 등록해두는 전역 입력 프로세서. NativeDestruct에서 해제한다 */
	TSharedPtr<IInputProcessor> AnyKeyInputProcessor;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 입력이 감지됐을 때 공통으로 처리하는 내부 함수 - 델리게이트 Broadcast + (설정된 경우) 위젯 전환 */
	void HandleAnyKeyPressed(FKey PressedKey);

public:

	/** 아무 입력이나 눌렸을 때 Broadcast. BP에서 자유롭게 바인딩해서 커스텀 전환 로직을 붙일 수 있다 */
	UPROPERTY(BlueprintAssignable, Category="UI Switch")
	FCPAnyKeyPressed OnAnyKeyPressed;
};

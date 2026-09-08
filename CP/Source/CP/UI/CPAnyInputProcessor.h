// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

/**
 *  포커스/히트테스트와 완전히 무관하게 키보드/마우스/게임패드의 눌림을 그대로 콜백으로 전달하는
 *  범용 입력 프로세서.
 *
 *  UCPPressAnyKeyWidget/UCPPlayerJoinWidget처럼 "화면 어디를 클릭했는지"나 "어떤 위젯이 키보드
 *  포커스를 가졌는지"와 무관하게 "아무 입력이나 눌렸는지"만 감지하면 되는 화면에 사용한다.
 *  NativeOnKeyDown/NativeOnMouseButtonDown + SetUserFocus 조합은 실제로 다음 두 경우에 동작하지
 *  않는 것이 로그로 확인됐다:
 *    - 마우스 클릭이 위젯의 히트테스트 가능한 영역을 맞히지 못하면 포커스 자체가 날아가버림
 *      (NativeOnFocusLost Cause=Mouse)
 *    - 게임패드 입력은 SetUserFocus가 쓰는 레거시 ControllerId 기반 Slate User와, 실제 게임패드
 *      키 이벤트가 라우팅되는 Slate User가 서로 어긋나 포커스가 있어도 이벤트가 안 옴
 *  IInputProcessor는 Slate가 포커스/히트테스트로 이벤트를 어디에 보낼지 정하기 "이전" 단계에서
 *  가로채므로 위 두 문제 모두와 무관하게 항상 동작한다.
 */
class FCPAnyInputProcessor : public IInputProcessor
{
public:

	using FOnKeyPressed = TFunction<void(const FKeyEvent&)>;
	using FOnMousePressed = TFunction<void(const FPointerEvent&)>;

	FCPAnyInputProcessor(FOnKeyPressed InOnKeyPressed, FOnMousePressed InOnMousePressed)
		: OnKeyPressed(MoveTemp(InOnKeyPressed))
		, OnMousePressed(MoveTemp(InOnMousePressed))
	{
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (OnKeyPressed)
		{
			OnKeyPressed(InKeyEvent);
		}

		// 소비하지 않고 그대로 흘려보내 다른 정상적인 입력 처리에 영향을 주지 않는다
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if (OnMousePressed)
		{
			OnMousePressed(MouseEvent);
		}

		return false;
	}

private:

	FOnKeyPressed OnKeyPressed;
	FOnMousePressed OnMousePressed;
};

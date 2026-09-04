// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPPressAnyKeyWidget.h"
#include "GameFramework/PlayerController.h"

void UCPPressAnyKeyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	// GetOwningPlayer()가 비어있을 수 있는 생성 경로(예: World Context만으로 CreateWidget)까지
	// 커버하기 위해 Player 0으로 폴백. 로비/타이틀 시점엔 어차피 Player 0만 존재하고, 배정 전
	// 게임패드 입력도 기본적으로 Player 0에게 온다 (ACPLobbyGameMode 참고)
	APlayerController* OwningController = GetOwningPlayer();
	if (!OwningController)
	{
		OwningController = GetWorld()->GetFirstPlayerController();
	}

	if (OwningController)
	{
		// Game Only 모드에서는 UMG가 키/게임패드 입력을 아예 받지 못하므로, 이 위젯이 뜬 동안은
		// UI에 포커스를 강제해 키보드/마우스/게임패드 입력이 전부 이 위젯으로 들어오게 한다
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		OwningController->SetInputMode(InputMode);

		SetUserFocus(OwningController);
	}
}

// TODO: FReply 사용 시 "unresolved external symbol FReply::FReply(bool)" 링크 에러 발생
// (CP.Build.cs에 SlateCore 모듈 의존성 누락이 원인) - 해결 전까지 주석 처리
//FReply UCPPressAnyKeyWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
//{
//	HandleAnyKeyPressed(InKeyEvent.GetKey());
//
//	return FReply::Handled();
//}
//
//FReply UCPPressAnyKeyWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
//{
//	HandleAnyKeyPressed(InMouseEvent.GetEffectingButton());
//
//	return FReply::Handled();
//}

void UCPPressAnyKeyWidget::HandleAnyKeyPressed(FKey PressedKey)
{
	OnAnyKeyPressed.Broadcast(PressedKey);

	if (NextWidgetClass)
	{
		if (UUserWidget* NextWidget = CreateWidget<UUserWidget>(GetWorld(), NextWidgetClass))
		{
			NextWidget->AddToViewport();
		}

		if (bRemoveSelfOnSwitch)
		{
			RemoveFromParent();
		}
	}
}

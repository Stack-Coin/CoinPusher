// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPPlayerJoinWidget.h"
#include "GameMode/CPLobbyGameMode.h"
#include "GameFramework/PlayerController.h"

void UCPPlayerJoinWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	// GetOwningPlayer()가 비어있을 수 있는 생성 경로까지 커버하기 위해 Player 0으로 폴백.
	// 참가 화면 시점엔 어차피 Player 0만 존재하고, 배정 전 게임패드 입력도 기본적으로
	// Player 0에게 온다 (ACPLobbyGameMode 참고)
	APlayerController* OwningController = GetOwningPlayer();
	if (!OwningController)
	{
		OwningController = GetWorld()->GetFirstPlayerController();
	}

	if (OwningController)
	{
		// Game Only 모드에서는 UMG가 키/게임패드 입력을 아예 받지 못하므로, 이 위젯이 뜬 동안은
		// UI에 포커스를 강제해 키보드/마우스/게임패드 입력이 전부 이 위젯으로 들어오게 한다.
		// 이게 안 되어 있으면 NativeOnKeyDown 자체가 호출되지 않아 RegisterPlayerInput도,
		// 그 결과인 OnPlayerSlotAssigned도 영영 호출되지 않는다
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		OwningController->SetInputMode(InputMode);

		SetUserFocus(OwningController);
	}

	if (ACPLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<ACPLobbyGameMode>())
	{
		LobbyGameMode->OnPlayerJoined.AddDynamic(this, &UCPPlayerJoinWidget::HandlePlayerJoined);
	}
}

// TODO: FReply 사용 시 "unresolved external symbol FReply::FReply(bool)" 링크 에러 발생
// (CP.Build.cs에 SlateCore 모듈 의존성 누락이 원인) - 해결 전까지 주석 처리
//FReply UCPPlayerJoinWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
//{
//	if (ACPLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<ACPLobbyGameMode>())
//	{
//		LobbyGameMode->RegisterPlayerInput(InKeyEvent.GetInputDeviceId());
//	}
//
//	return FReply::Handled();
//}
//
//FReply UCPPlayerJoinWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
//{
//	if (ACPLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<ACPLobbyGameMode>())
//	{
//		LobbyGameMode->RegisterPlayerInput(InMouseEvent.GetInputDeviceId());
//	}
//
//	return FReply::Handled();
//}

void UCPPlayerJoinWidget::HandlePlayerJoined(int32 PlayerIndex)
{
	OnPlayerSlotAssigned(PlayerIndex);
}

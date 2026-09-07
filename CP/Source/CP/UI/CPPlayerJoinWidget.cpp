// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPPlayerJoinWidget.h"
#include "UI/CPAnyInputProcessor.h"
#include "GameMode/CPLobbyGameMode.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/Image.h"

void UCPPlayerJoinWidget::NativeConstruct()
{
	Super::NativeConstruct();

	TWeakObjectPtr<UCPPlayerJoinWidget> WeakThis(this);

	AnyKeyInputProcessor = MakeShared<FCPAnyInputProcessor>(
		[WeakThis](const FKeyEvent& KeyEvent)
		{
			if (UCPPlayerJoinWidget* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleAnyInputPressed(KeyEvent.GetInputDeviceId());
			}
		},
		[WeakThis](const FPointerEvent& MouseEvent)
		{
			if (UCPPlayerJoinWidget* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleAnyInputPressed(MouseEvent.GetInputDeviceId());
			}
		});

	FSlateApplication::Get().RegisterInputPreProcessor(AnyKeyInputProcessor);

	if (ACPLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<ACPLobbyGameMode>())
	{
		LobbyGameMode->OnPlayerJoined.AddDynamic(this, &UCPPlayerJoinWidget::HandlePlayerJoined);
	}
}

void UCPPlayerJoinWidget::NativeDestruct()
{
	if (AnyKeyInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(AnyKeyInputProcessor);
	}
	AnyKeyInputProcessor.Reset();

	Super::NativeDestruct();
}

void UCPPlayerJoinWidget::HandleAnyInputPressed(FInputDeviceId DeviceId)
{
	if (ACPLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<ACPLobbyGameMode>())
	{
		LobbyGameMode->RegisterPlayerInput(DeviceId);
	}
}

void UCPPlayerJoinWidget::HandlePlayerJoined(int32 PlayerIndex)
{
	OnPlayerSlotAssigned(PlayerIndex);
}

void UCPPlayerJoinWidget::OnPlayerSlotAssigned_Implementation(int32 PlayerIndex)
{
	if (PlayerIndex == 0 && Player1Square)
	{
		Player1Square->SetColorAndOpacity(Player1Color);
	}
	else if (PlayerIndex == 1 && Player2Square)
	{
		Player2Square->SetColorAndOpacity(Player2Color);
	}
}

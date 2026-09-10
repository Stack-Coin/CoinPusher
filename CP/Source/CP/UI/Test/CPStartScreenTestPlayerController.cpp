// Fill out your copyright notice in the Description page of Project Settings.

#include "CPStartScreenTestPlayerController.h"
#include "GameMode/CPControllerTypeSubsystem.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "UObject/Class.h"

ACPStartScreenTestPlayerController::ACPStartScreenTestPlayerController()
{
	// 이 화면은 키보드/게임패드로 "아무 버튼이나" 누르는 것이 주 입력이라 마우스 커서는 꺼둔다
	bShowMouseCursor = false;
}

void ACPStartScreenTestPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ACPStartScreenTestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::AnyKey, IE_Pressed, this, &ACPStartScreenTestPlayerController::HandleAnyKeyPressed_Debug);
	}
}

void ACPStartScreenTestPlayerController::HandleAnyKeyPressed_Debug()
{
	UGameInstance* GameInstance = GetGameInstance();
	UCPControllerTypeSubsystem* ControllerTypeSubsystem = GameInstance ? GameInstance->GetSubsystem<UCPControllerTypeSubsystem>() : nullptr;
	if (!ControllerTypeSubsystem)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ACPStartScreenTestPlayerController] Input detected - SelectedControllerType: %s"),
		*UEnum::GetValueAsString(ControllerTypeSubsystem->GetSelectedControllerType()));
}

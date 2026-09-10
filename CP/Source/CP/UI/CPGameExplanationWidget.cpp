// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPGameExplanationWidget.h"
#include "GameMode/CPControllerTypeSubsystem.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

void UCPGameExplanationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ECPControllerType SelectedControllerType = ECPControllerType::KeyboardMouse;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCPControllerTypeSubsystem* ControllerTypeSubsystem = GameInstance->GetSubsystem<UCPControllerTypeSubsystem>())
		{
			SelectedControllerType = ControllerTypeSubsystem->GetSelectedControllerType();
		}
	}

	const bool bIsGamepad = (SelectedControllerType == ECPControllerType::GamePad);

	if (GamePadImage)
	{
		GamePadImage->SetVisibility(bIsGamepad ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (KeyBoardImage)
	{
		KeyBoardImage->SetVisibility(bIsGamepad ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	OnAnyKeyPressed.AddDynamic(this, &UCPGameExplanationWidget::HandleSelectedControllerKeyPressed);
}

void UCPGameExplanationWidget::HandleSelectedControllerKeyPressed(FKey PressedKey)
{
	if (bHasRequestedLevelChange)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UCPControllerTypeSubsystem* ControllerTypeSubsystem = GameInstance ? GameInstance->GetSubsystem<UCPControllerTypeSubsystem>() : nullptr;
	if (!ControllerTypeSubsystem)
	{
		return;
	}

	const bool bSelectedIsGamepad = (ControllerTypeSubsystem->GetSelectedControllerType() == ECPControllerType::GamePad);

	// 선택된 컨트롤러와 다른 종류의 입력(예: 게임패드를 선택했는데 키보드를 누름)은 무시
	if (PressedKey.IsGamepadKey() != bSelectedIsGamepad)
	{
		return;
	}

	if (NextLevelName.IsNone())
	{
		return;
	}

	bHasRequestedLevelChange = true;

	UGameplayStatics::OpenLevel(this, NextLevelName);
}

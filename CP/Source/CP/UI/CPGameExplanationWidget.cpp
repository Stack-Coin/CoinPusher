// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPGameExplanationWidget.h"
#include "GameMode/CPControllerTypeSubsystem.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

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

void UCPGameExplanationWidget::ScheduleSwitchToNextWidget()
{
	// 아무 것도 하지 않음 - 부모가 매 입력마다 자동으로 부르지만, 이 위젯은 컨트롤러 종류가
	// 일치하는 입력에서만 전환해야 하므로 HandleSelectedControllerKeyPressed가 직접 처리한다
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

	bHasRequestedLevelChange = true;

	PlayClickSound();
	SwitchToNextWidget();
}

void UCPGameExplanationWidget::PlayClickSound() const
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}
}

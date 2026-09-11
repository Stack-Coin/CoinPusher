// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPInGamePauseWidget.h"
#include "GameMode/CPControllerType.h"
#include "GameMode/CPControllerTypeSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UCPInGamePauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	NavigableButtons.Reset();
	ButtonOutlines.Reset();

	if (EndGameButton)
	{
		NavigableButtons.Add(EndGameButton);
		ButtonOutlines.Add(EndGameButtonOutline);
		EndGameButton->OnHovered.AddDynamic(this, &UCPInGamePauseWidget::HandleEndGameButtonHovered);
		EndGameButton->OnClicked.AddDynamic(this, &UCPInGamePauseWidget::HandleEndGameButtonClicked);
	}

	if (ReturnToTitleButton)
	{
		NavigableButtons.Add(ReturnToTitleButton);
		ButtonOutlines.Add(ReturnToTitleButtonOutline);
		ReturnToTitleButton->OnHovered.AddDynamic(this, &UCPInGamePauseWidget::HandleReturnToTitleButtonHovered);
		ReturnToTitleButton->OnClicked.AddDynamic(this, &UCPInGamePauseWidget::HandleReturnToTitleButtonClicked);
	}

	RefreshForDisplay();
}

void UCPInGamePauseWidget::RefreshForDisplay()
{
	RefreshBackgroundForControllerType();

	SelectedButtonIndex = 0;
	UpdateSelectionVisuals();
}

void UCPInGamePauseWidget::RefreshBackgroundForControllerType()
{
	ECPControllerType ControllerType = ECPControllerType::KeyboardMouse;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCPControllerTypeSubsystem* ControllerTypeSubsystem = GameInstance->GetSubsystem<UCPControllerTypeSubsystem>())
		{
			ControllerType = ControllerTypeSubsystem->GetSelectedControllerType();
		}
	}

	const bool bIsGamePad = (ControllerType == ECPControllerType::GamePad);

	if (KeyboardMouseBackgroundImage)
	{
		KeyboardMouseBackgroundImage->SetVisibility(bIsGamePad ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	if (GamePadBackgroundImage)
	{
		GamePadBackgroundImage->SetVisibility(bIsGamePad ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCPInGamePauseWidget::UpdateSelectionVisuals()
{
	for (int32 Index = 0; Index < ButtonOutlines.Num(); ++Index)
	{
		if (UWidget* Outline = ButtonOutlines[Index])
		{
			Outline->SetVisibility(Index == SelectedButtonIndex ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UCPInGamePauseWidget::SetSelectedButtonIndex(int32 NewIndex)
{
	if (!NavigableButtons.IsValidIndex(NewIndex))
	{
		return;
	}

	SelectedButtonIndex = NewIndex;
	UpdateSelectionVisuals();
}

void UCPInGamePauseWidget::MoveSelection(int32 Delta)
{
	const int32 Count = NavigableButtons.Num();
	if (Count == 0)
	{
		return;
	}

	SelectedButtonIndex = ((SelectedButtonIndex + Delta) % Count + Count) % Count;
	UpdateSelectionVisuals();
}

void UCPInGamePauseWidget::ConfirmSelection()
{
	ExecuteButtonAction(SelectedButtonIndex);
}

void UCPInGamePauseWidget::ExecuteButtonAction(int32 Index)
{
	if (!NavigableButtons.IsValidIndex(Index))
	{
		return;
	}

	UButton* Button = NavigableButtons[Index];

	if (Button == EndGameButton)
	{
		EndGame();
	}
	else if (Button == ReturnToTitleButton)
	{
		ReturnToTitle();
	}
}

void UCPInGamePauseWidget::HandleEndGameButtonHovered()
{
	SetSelectedButtonIndex(NavigableButtons.IndexOfByKey(EndGameButton));
}

void UCPInGamePauseWidget::HandleEndGameButtonClicked()
{
	ExecuteButtonAction(NavigableButtons.IndexOfByKey(EndGameButton));
}

void UCPInGamePauseWidget::HandleReturnToTitleButtonHovered()
{
	SetSelectedButtonIndex(NavigableButtons.IndexOfByKey(ReturnToTitleButton));
}

void UCPInGamePauseWidget::HandleReturnToTitleButtonClicked()
{
	ExecuteButtonAction(NavigableButtons.IndexOfByKey(ReturnToTitleButton));
}

void UCPInGamePauseWidget::EndGame()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

void UCPInGamePauseWidget::ReturnToTitle()
{
	if (!TitleLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, TitleLevelName);
	}
}

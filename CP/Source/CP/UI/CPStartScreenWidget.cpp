// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPStartScreenWidget.h"
#include "GameMode/CPControllerTypeSubsystem.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"

void UCPStartScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OnAnyKeyPressed.AddDynamic(this, &UCPStartScreenWidget::HandleControllerInputDetected);
}

void UCPStartScreenWidget::HandleControllerInputDetected(FKey PressedKey)
{
	// 점멸(전환 시퀀스)이 이미 시작된 뒤에는 선택을 더 바꾸지 않는다
	if (bHasStartedBlink)
	{
		return;
	}

	const bool bIsGamepad = PressedKey.IsGamepadKey();

	if (KeyBoardImage)
	{
		KeyBoardImage->SetVisibility(bIsGamepad ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	if (GamePadImage)
	{
		GamePadImage->SetVisibility(bIsGamepad ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCPControllerTypeSubsystem* ControllerTypeSubsystem = GameInstance->GetSubsystem<UCPControllerTypeSubsystem>())
		{
			ControllerTypeSubsystem->SetSelectedControllerType(bIsGamepad ? ECPControllerType::GamePad : ECPControllerType::KeyboardMouse);
		}
	}
}

void UCPStartScreenWidget::ScheduleSwitchToNextWidget()
{
	if (bHasStartedBlink || !NextWidgetClass)
	{
		return;
	}

	bHasStartedBlink = true;

	// HandleControllerInputDetected가 방금 갱신해둔 상태 그대로, 숨겨지지 않은(=선택된) 아이콘을 점멸시킨다
	BlinkingImage = (GamePadImage && GamePadImage->GetVisibility() != ESlateVisibility::Collapsed) ? GamePadImage : KeyBoardImage;
	BlinkToggleCount = 0;

	if (!BlinkingImage)
	{
		// 점멸시킬 아이콘이 없으면(WBP에 안 붙여둔 경우) 바로 전환
		SwitchToNextWidget();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BlinkTimerHandle, this, &UCPStartScreenWidget::HandleBlinkTick, BlinkInterval, true);
	}
}

void UCPStartScreenWidget::HandleBlinkTick()
{
	++BlinkToggleCount;

	// 짝수 번째 토글마다 다시 보이도록 - 선택된 아이콘은 이미 보이는 상태로 시작하므로
	// 1번째=숨김, 2번째=보임, ... 으로 BlinkCount번 깜빡인 뒤(=토글 BlinkCount*2번) 보이는 채로 끝난다
	const bool bShouldBeVisible = (BlinkToggleCount % 2 == 0);
	BlinkingImage->SetVisibility(bShouldBeVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (BlinkToggleCount < BlinkCount * 2)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlinkTimerHandle);
	}

	SwitchToNextWidget();
}

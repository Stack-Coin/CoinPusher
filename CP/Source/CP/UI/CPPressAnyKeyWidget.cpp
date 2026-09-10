// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPPressAnyKeyWidget.h"
#include "UI/CPAnyInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "TimerManager.h"

void UCPPressAnyKeyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	TWeakObjectPtr<UCPPressAnyKeyWidget> WeakThis(this);

	AnyKeyInputProcessor = MakeShared<FCPAnyInputProcessor>(
		[WeakThis](const FKeyEvent& KeyEvent)
		{
			if (UCPPressAnyKeyWidget* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleAnyKeyPressed(KeyEvent.GetKey());
			}
		},
		[WeakThis](const FPointerEvent& MouseEvent)
		{
			if (UCPPressAnyKeyWidget* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleAnyKeyPressed(MouseEvent.GetEffectingButton());
			}
		});

	FSlateApplication::Get().RegisterInputPreProcessor(AnyKeyInputProcessor);
}

void UCPPressAnyKeyWidget::NativeDestruct()
{
	if (AnyKeyInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(AnyKeyInputProcessor);
	}
	AnyKeyInputProcessor.Reset();

	Super::NativeDestruct();
}

void UCPPressAnyKeyWidget::HandleAnyKeyPressed(FKey PressedKey)
{
	OnAnyKeyPressed.Broadcast(PressedKey);

	ScheduleSwitchToNextWidget();
}

void UCPPressAnyKeyWidget::ScheduleSwitchToNextWidget()
{
	if (!NextWidgetClass)
	{
		return;
	}

	if (SwitchDelay <= 0.0f)
	{
		SwitchToNextWidget();
		return;
	}

	// 대기 중에 추가로 입력이 들어와도 최초 입력 기준 딜레이를 그대로 유지 (다시 잡지 않음)
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(SwitchTimerHandle))
		{
			World->GetTimerManager().SetTimer(SwitchTimerHandle, this, &UCPPressAnyKeyWidget::SwitchToNextWidget, SwitchDelay, false);
		}
	}
}

void UCPPressAnyKeyWidget::SwitchToNextWidget()
{
	if (NextWidgetClass)
	{
		if (UUserWidget* NextWidget = CreateWidget<UUserWidget>(GetWorld(), NextWidgetClass))
		{
			NextWidget->AddToViewport();
		}
	}

	if (bRemoveSelfOnSwitch)
	{
		RemoveFromParent();
	}
}

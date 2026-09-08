// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CPPressAnyKeyWidget.h"
#include "UI/CPAnyInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

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

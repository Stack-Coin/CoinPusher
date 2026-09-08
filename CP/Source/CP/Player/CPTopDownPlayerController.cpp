// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPTopDownPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Blueprint/UserWidget.h"
#include "Debug/CPDebugWidget.h"

ACPTopDownPlayerController::ACPTopDownPlayerController()
{
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ACPTopDownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Always show the mouse pointer in-game (regardless of which input device this player uses)
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

bool ACPTopDownPlayerController::IsUsingKeyboardAndMouse() const
{
	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();

	TArray<FInputDeviceId> OwnedDevices;
	DeviceMapper.GetAllInputDevicesForUser(GetPlatformUserId(), OwnedDevices);

	return OwnedDevices.Contains(DeviceMapper.GetDefaultInputDevice());
}

void ACPTopDownPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(ToggleDebugWidgetAction, ETriggerEvent::Started, this, &ACPTopDownPlayerController::ToggleDebugWidget);
	}
}

void ACPTopDownPlayerController::ToggleDebugWidget(const FInputActionValue& Value)
{
	// 디버그용 기능이라 어떤 플레이어(장치)가 눌렀는지와 무관하게 항상 동작해야 함 - 예전에는
	// IsUsingKeyboardAndMouse()로 막아서 키보드/마우스를 소유하지 않은 플레이어는 토글이 안 됐음
	if (!DebugWidgetClass)
	{
		return;
	}

	if (!DebugWidgetInstance)
	{
		DebugWidgetInstance = CreateWidget<UCPDebugWidget>(this, DebugWidgetClass);
		if (DebugWidgetInstance)
		{
			DebugWidgetInstance->AddToViewport(10);
			DebugWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!DebugWidgetInstance)
	{
		return;
	}

	const bool bIsVisible = DebugWidgetInstance->GetVisibility() == ESlateVisibility::Visible;
	DebugWidgetInstance->SetVisibility(bIsVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

bool ACPTopDownPlayerController::GetCursorWorldLocation(FVector& OutWorldLocation) const
{
	FHitResult CursorHit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, CursorHit))
	{
		OutWorldLocation = CursorHit.Location;
		return true;
	}

	return false;
}

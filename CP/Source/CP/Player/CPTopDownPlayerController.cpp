// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPTopDownPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"

ACPTopDownPlayerController::ACPTopDownPlayerController()
{
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ACPTopDownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// A gamepad-mapped player (see ACPGameMode::BeginPlay) has no meaningful cursor position, so only
	// show/aim-with the mouse cursor for the player who still owns the keyboard/mouse device
	bShowMouseCursor = IsUsingKeyboardAndMouse();

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

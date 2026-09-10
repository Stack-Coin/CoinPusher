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

#include "Kismet/GameplayStatics.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "CoinPusher/CPCoinPusherCaptureWidget.h"

ACPTopDownPlayerController::ACPTopDownPlayerController()
{
	CaptureWidgetClass = UCPCoinPusherCaptureWidget::StaticClass();
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

	GetWorldTimerManager().SetTimerForNextTick(this, &ACPTopDownPlayerController::SetupCaptureWidget);
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






//캡처 위젯 설정

namespace
{
	// 전용 테스트 액터(ACPCoinPusherCaptureTestActor)가 없는 레벨(예: 실제 BP_CoinPusher가 배치된
	// 테스트 레벨)에서도 동작하도록, 없으면 실제 ACPCoinPusher를 대신 찾는다
	AActor* FindCaptureSourceActor(const UObject* WorldContextObject)
	{
		return UGameplayStatics::GetActorOfClass(WorldContextObject, ACPCoinPusher::StaticClass());
	}

	UCPCoinPusherViewCaptureComponent* GetCaptureComponent(AActor* SourceActor)
	{
		if (ACPCoinPusher* CoinPusher = Cast<ACPCoinPusher>(SourceActor))
		{
			return CoinPusher->GetViewCaptureComponent();
		}

		return nullptr;
	}
}


void ACPTopDownPlayerController::SetupCaptureWidget()
{
	if (!CaptureWidgetClass)
	{
		return;
	}

	UCPCoinPusherViewCaptureComponent* CaptureComponent = GetCaptureComponent(FindCaptureSourceActor(this));
	if (!CaptureComponent)
	{
		return;
	}

	UCPCoinPusherCaptureWidget* CaptureWidget = CreateWidget<UCPCoinPusherCaptureWidget>(this, CaptureWidgetClass);
	if (!CaptureWidget)
	{
		return;
	}

	// UCPCoinPusherViewportClient가 Player 카메라를 오른쪽으로 축소해뒀으므로, 이 위젯은
	// AddToPlayerScreen이 아니라 뷰포트 전체 기준으로 추가되어야 왼쪽 영역까지 그릴 수 있다
	CaptureWidget->AddToViewport(0);
	CaptureWidget->SetCaptureTexture(CaptureComponent->GetViewRenderTarget());
}


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
#include "UI/CPInGamePauseWidget.h"
#include "UI/CPEndingWidget.h"

#include "Kismet/GameplayStatics.h"
#include "CoinPusher/CPCoinPusher.h"
#include "CoinPusher/CPCoinPusherViewCaptureComponent.h"
#include "CoinPusher/CPCoinPusherCaptureWidget.h"
#include "Log/CPLogCategories.h"

ACPTopDownPlayerController::ACPTopDownPlayerController()
{
	CaptureWidgetClass = UCPCoinPusherCaptureWidget::StaticClass();
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	// UGameplayStatics::SetGamePaused(true)로 일시정지하면 APlayerController::TickActor()가 기본적으로
	// PlayerTick()(=Enhanced Input의 액션 평가/바인딩된 델리게이트 호출 경로)을 건너뛴다 - 이 플래그가
	// false인 채로는 일시정지 중 PauseAction(재개)/MenuNavigateAction/MenuConfirmAction이 전혀 호출되지
	// 않는다. 단, Input Action 에셋 쪽의 bTriggerWhenPaused도 각각 true로 켜야 한다(에디터에서 설정,
	// UI/README.md 참고) - 이 플래그는 PlayerTick 자체가 도는지만 결정하고, 실제로 어떤 액션이 일시정지
	// 중에도 트리거될지는 bTriggerWhenPaused가 따로 결정한다
	bShouldPerformFullTickWhenPaused = true;
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
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ACPTopDownPlayerController::TogglePauseMenu);
		EnhancedInputComponent->BindAction(MenuNavigateAction, ETriggerEvent::Triggered, this, &ACPTopDownPlayerController::HandleMenuNavigate);
		EnhancedInputComponent->BindAction(MenuConfirmAction, ETriggerEvent::Started, this, &ACPTopDownPlayerController::HandleMenuConfirm);
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

ACPTopDownPlayerController* ACPTopDownPlayerController::GetMenuOwnerController() const
{
	UWorld* World = GetWorld();
	return World ? Cast<ACPTopDownPlayerController>(World->GetFirstPlayerController()) : nullptr;
}

UCPInGamePauseWidget* ACPTopDownPlayerController::GetActiveMenuWidget() const
{
	ACPTopDownPlayerController* OwnerPC = GetMenuOwnerController();
	if (!OwnerPC)
	{
		return nullptr;
	}

	if (OwnerPC->EndingWidgetInstance && OwnerPC->EndingWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return OwnerPC->EndingWidgetInstance;
	}

	if (OwnerPC->PauseWidgetInstance && OwnerPC->PauseWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return OwnerPC->PauseWidgetInstance;
	}

	return nullptr;
}

void ACPTopDownPlayerController::TogglePauseMenu(const FInputActionValue& Value)
{
	ACPTopDownPlayerController* OwnerPC = GetMenuOwnerController();
	if (!OwnerPC || !OwnerPC->InGamePauseWidgetClass)
	{
		return;
	}

	// Ending 화면이 떠 있으면(게임이 끝난 상태) 별도로 일시정지 메뉴를 열지 않는다 - Ending은
	// 게임 종료/타이틀로 버튼만 제공하고 재개(resume) 개념이 없음
	if (OwnerPC->EndingWidgetInstance && OwnerPC->EndingWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return;
	}

	const bool bNewPausedState = !UGameplayStatics::IsGamePaused(this);
	UGameplayStatics::SetGamePaused(this, bNewPausedState);
	OwnerPC->SetPauseMenuVisible(bNewPausedState);
}

void ACPTopDownPlayerController::SetPauseMenuVisible(bool bVisible)
{
	if (bVisible)
	{
		if (!PauseWidgetInstance && InGamePauseWidgetClass)
		{
			PauseWidgetInstance = CreateWidget<UCPInGamePauseWidget>(this, InGamePauseWidgetClass);
			if (PauseWidgetInstance)
			{
				PauseWidgetInstance->AddToViewport(20);
			}
		}

		if (!PauseWidgetInstance)
		{
			return;
		}

		PauseWidgetInstance->RefreshForDisplay();
		PauseWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
	else if (PauseWidgetInstance)
	{
		PauseWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ACPTopDownPlayerController::ShowEndingResult(bool bIsClear)
{
	ACPTopDownPlayerController* OwnerPC = GetMenuOwnerController();
	if (!OwnerPC || !OwnerPC->EndingWidgetClass)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(this, true);
	OwnerPC->SetEndingMenuVisible(bIsClear);
}

void ACPTopDownPlayerController::SetEndingMenuVisible(bool bIsClear)
{
	if (!EndingWidgetInstance && EndingWidgetClass)
	{
		EndingWidgetInstance = CreateWidget<UCPEndingWidget>(this, EndingWidgetClass);
		if (EndingWidgetInstance)
		{
			EndingWidgetInstance->AddToViewport(20);
		}
	}

	if (!EndingWidgetInstance)
	{
		return;
	}

	// InGamePause 메뉴가 열려 있었다면 Ending 화면으로 교체
	SetPauseMenuVisible(false);

	EndingWidgetInstance->RefreshForDisplay();
	EndingWidgetInstance->ShowResult(bIsClear);
	EndingWidgetInstance->SetVisibility(ESlateVisibility::Visible);
}

void ACPTopDownPlayerController::HandleMenuNavigate(const FInputActionValue& Value)
{
	// 임시 진단 로그 - MenuNavigateAction이 아예 안 불리는지(바인딩/Trigger When Paused 문제),
	// 불리긴 하는데 X가 계속 0인지(IMC에서 실제로 매핑된 키/Swizzle 문제)를 Output Log로 구분하기
	// 위한 것. 원인 확인 후 제거해도 됨
	UE_LOG(LogPlayer, Warning, TEXT("HandleMenuNavigate raw value = %s"), *Value.Get<FVector2D>().ToString());

	UCPInGamePauseWidget* ActiveMenu = GetActiveMenuWidget();
	if (!ActiveMenu)
	{
		bHasProcessedMenuNavigateThisHold = false;
		return;
	}

	// 버튼들이 화면에 가로로(EndGameButton/ReturnToTitleButton) 배치되므로 좌우(X축)로 선택을 옮긴다
	const float AxisX = Value.Get<FVector2D>().X;
	if (FMath::Abs(AxisX) < MenuNavigateDeadZone)
	{
		bHasProcessedMenuNavigateThisHold = false;
		return;
	}

	if (bHasProcessedMenuNavigateThisHold)
	{
		return;
	}

	bHasProcessedMenuNavigateThisHold = true;

	// 스틱 왼쪽(-X)은 이전 버튼(-1), 오른쪽(+X)은 다음 버튼(+1)으로 이동
	ActiveMenu->MoveSelection(AxisX > 0.0f ? 1 : -1);
}

void ACPTopDownPlayerController::HandleMenuConfirm(const FInputActionValue& Value)
{
	if (UCPInGamePauseWidget* ActiveMenu = GetActiveMenuWidget())
	{
		ActiveMenu->ConfirmSelection();
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


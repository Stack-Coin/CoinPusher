// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CPLobbyPlayerController.h"
#include "Components/InputComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"

namespace
{
	/** 디버그 전용 입력 프로세서. Slate가 포커스/입력 모드(Game Only/UI Only/Game And UI)에 따라
	 *  이벤트를 어디로 보낼지 정하는 것보다 "먼저" 호출되므로, PlayerController의 InputComponent
	 *  바인딩(BindKey)이나 위젯의 NativeOnKeyDown이 입력 모드 때문에 아예 안 불리는 경우와 달리,
	 *  이 로그는 입력 모드/포커스와 무관하게 OS에서 게임 창까지 입력이 들어오기만 하면 찍힌다.
	 *  즉 "입력이 엔진까지 도달하는지" 자체를 확인하는 가장 낮은 단계의 체크포인트 */
	class FCPDebugInputLogger : public IInputProcessor
	{
	public:
		virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
			UE_LOG(LogTemp, Warning, TEXT("[RawInput] KeyDown: %s"), *InKeyEvent.GetKey().ToString());
			return false; // 소비하지 않고 그대로 흘려보내 정상적인 라우팅에 영향을 주지 않는다
		}

		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
		{
			UE_LOG(LogTemp, Warning, TEXT("[RawInput] MouseButtonDown: %s"), *MouseEvent.GetEffectingButton().ToString());
			return false;
		}
	};
}

ACPLobbyPlayerController::ACPLobbyPlayerController()
{
	// 이 화면은 키보드/게임패드로 "아무 버튼이나" 누르는 것이 주 입력이라 마우스 커서는 꺼둔다.
	// (Press Any Key / Player Join 위젯이 각자 필요한 입력 모드/포커스를 직접 설정)
	bShowMouseCursor = false;
}

void ACPLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 기본 입력 모드를 UI 우호적으로 깔아둔다 - 위젯이 자기 NativeConstruct에서 한 프레임 뒤에
	// FInputModeUIOnly로 다시 덮어쓰겠지만, 그 전까지의 공백을 없애고 다른 경로로 위젯이 뜨는
	// 경우에도 최소한 UMG가 입력을 받을 수 있는 상태를 보장한다
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// 디버그: 입력 모드/포커스와 무관하게 OS 입력이 게임까지 도달하는지 확인
	FSlateApplication::Get().RegisterInputPreProcessor(MakeShared<FCPDebugInputLogger>());
}

void ACPLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPLobbyPlayerController: 	Super::SetupInputComponent();"));

		InputComponent->BindKey(EKeys::AnyKey, IE_Pressed, this, &ACPLobbyPlayerController::HandleAnyKeyPressed_Debug);
	}
}

void ACPLobbyPlayerController::HandleAnyKeyPressed_Debug()
{
	UE_LOG(LogTemp, Warning, TEXT("ACPLobbyPlayerController: key/mouse/pad input detected"));
}

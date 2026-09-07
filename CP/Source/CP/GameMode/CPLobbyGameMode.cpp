// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CPLobbyGameMode.h"
#include "GameMode/CPLobbyPlayerController.h"
#include "GameMode/CPPlayerRegistrySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"

ACPLobbyGameMode::ACPLobbyGameMode()
{
	PlayerControllerClass = ACPLobbyPlayerController::StaticClass();

	// 순수 UI 화면이라 조종할 Pawn이 필요 없음. 비워두지 않으면 프로젝트 기본 Pawn 클래스가
	// 그대로 상속돼 이 로비 레벨에서도 Pawn이 스폰/Possess되고, 그 Pawn/Controller 쪽 초기화
	// 로직(Enhanced Input 요구 등)이 위젯의 UI 입력 모드/포커스와 불필요하게 얽힐 수 있다
	DefaultPawnClass = nullptr;
}

void ACPLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!StartWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPLobbyGameMode::BeginPlay - StartWidgetClass가 지정되지 않아 시작 화면을 띄우지 않습니다. 이 GameMode를 상속하는 BP의 Class Defaults에서 StartWidgetClass를 지정하세요."));
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPLobbyGameMode::BeginPlay - Player 0의 PlayerController를 찾지 못해 시작 화면을 띄우지 못했습니다."));
		return;
	}

	if (UUserWidget* StartWidget = CreateWidget<UUserWidget>(PC, StartWidgetClass))
	{
		StartWidget->AddToViewport();
	}
}

void ACPLobbyGameMode::RegisterPlayerInput(FInputDeviceId DeviceId)
{
	if (!DeviceId.IsValid() || ClaimedDeviceIds.Contains(DeviceId) || ClaimedDeviceIds.Num() >= NumberOfPlayersToJoin)
	{
		return;
	}

	// 지금까지 배정된 장치 수를 그대로 PlayerIndex로 사용 - 이 함수가 가장 먼저 호출된 장치가
	// PlayerIndex 0(1P)이 되고, 이미 존재하던 Player 0 컨트롤러와는 무관하다
	const int32 PlayerIndex = ClaimedDeviceIds.Add(DeviceId);

	if (UCPPlayerRegistrySubsystem* PlayerRegistry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCPPlayerRegistrySubsystem>() : nullptr)
	{
		// GameInstance에 붙어있어 다음 레벨(OpenLevel 이후)에서도 "이 장치가 몇 번째 플레이어인지"를
		// 조회할 수 있다. 로컬 플레이어 생성/장치 리매핑은 여기서 하지 않고 다음 레벨에 맡긴다
		PlayerRegistry->RegisterJoinedPlayer(DeviceId);
	}

	OnPlayerJoined.Broadcast(PlayerIndex);

	if (ClaimedDeviceIds.Num() >= NumberOfPlayersToJoin)
	{
		OnAllPlayersJoined.Broadcast();

		GetWorldTimerManager().SetTimer(LevelLoadTimerHandle, this, &ACPLobbyGameMode::OpenNextLevel, LevelLoadDelay, false);
	}
}

void ACPLobbyGameMode::OpenNextLevel()
{
	if (!NextLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}

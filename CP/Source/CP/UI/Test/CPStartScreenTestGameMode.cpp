// Fill out your copyright notice in the Description page of Project Settings.

#include "CPStartScreenTestGameMode.h"
#include "CPStartScreenTestPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

ACPStartScreenTestGameMode::ACPStartScreenTestGameMode()
{
	PlayerControllerClass = ACPStartScreenTestPlayerController::StaticClass();

	// 순수 UI 테스트라 조종할 Pawn이 필요 없음 - ACPLobbyGameMode와 동일한 이유
	DefaultPawnClass = nullptr;
}

void ACPStartScreenTestGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!StartWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPStartScreenTestGameMode::BeginPlay - StartWidgetClass가 지정되지 않아 시작 화면을 띄우지 않습니다. 이 GameMode를 상속하는 BP의 Class Defaults에서 StartWidgetClass를 지정하세요."));
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPStartScreenTestGameMode::BeginPlay - Player 0의 PlayerController를 찾지 못해 시작 화면을 띄우지 못했습니다."));
		return;
	}

	if (UUserWidget* StartWidget = CreateWidget<UUserWidget>(PC, StartWidgetClass))
	{
		StartWidget->AddToViewport();
	}
}

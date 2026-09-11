// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPTopDownPlayerController.h"
#include "CPCoinPusherItemSpawnTestPlayerController.generated.h"

/**
 *  ACPCoinPusherItemSpawnTestPawn/GameMode와 함께 InGamePause/Ending UI 및 "InGameUI" HUD를
 *  실제 게임과 동일한 코드 경로(ACPTopDownPlayerController의 PauseAction/MenuNavigateAction/
 *  MenuConfirmAction/InGamePauseWidgetClass/EndingWidgetClass/ShowEndingResult/InGameWidgetClass/
 *  GetInGameWidget())로 테스트하기 위한 PlayerController. 이 클래스 자체는 별도 로직을 추가하지
 *  않는다 - 전부 ACPTopDownPlayerController에 이미 구현되어 있으며, 이 클래스는 그것을 상속하는
 *  BP(예: BP_CPCoinPusherItemSpawnTestPlayerController)를 만들어 Input Mapping Context/Input
 *  Action/위젯 클래스 값들을 채워 넣기 위한 지점일 뿐이다(값들이 BP에서 채워져야 하는 이유는
 *  ACPTopDownPlayerController 자체가 UCLASS(abstract)이기 때문 - Input Actions/WBP 클래스 참조는
 *  C++에 하드코딩할 수 없다). ACPCoinPusherItemSpawnTestGameMode를 상속하는 BP에서
 *  PlayerControllerClass를 그 BP로 지정해서 쓴다.
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestPlayerController : public ACPTopDownPlayerController
{
	GENERATED_BODY()
};

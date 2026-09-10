// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPTopDownPlayerController.h"
#include "CPCoinPusherItemSpawnTestPlayerController.generated.h"

/**
 *  ACPCoinPusherItemSpawnTestPawn/GameMode와 함께 InGamePause/Ending UI를 실제 게임과 동일한 코드
 *  경로(ACPTopDownPlayerController의 PauseAction/MenuNavigateAction/MenuConfirmAction/
 *  InGamePauseWidgetClass/EndingWidgetClass/ShowEndingResult)로 테스트하기 위한 PlayerController.
 *  ACPTopDownPlayerController 자체가 Input Mapping Context/Input Action/위젯 클래스를 BP에서 채워
 *  넣어야 하는 UCLASS(abstract)이므로, 이 클래스를 상속하는 BP(예:
 *  BP_CPCoinPusherItemSpawnTestPlayerController)를 만들어 그 값들을 지정한 뒤
 *  ACPCoinPusherItemSpawnTestGameMode를 상속하는 BP에서 PlayerControllerClass를 그 BP로 지정해서 쓴다.
 */
UCLASS()
class CP_API ACPCoinPusherItemSpawnTestPlayerController : public ACPTopDownPlayerController
{
	GENERATED_BODY()
};

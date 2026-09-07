// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CPLobbyPlayerController.generated.h"

/**
 *  로컬 참가 로비 화면(StartLevel 등)에서 쓰는 최소 구성 PlayerController.
 *  실제 게임플레이용 ACPTopDownPlayerController와 달리 Input Mapping Context를 추가하지 않는다 -
 *  이 화면의 입력은 UCPPressAnyKeyWidget/UCPPlayerJoinWidget이 NativeOnKeyDown으로 직접 처리하고,
 *  각 위젯이 자기 NativeConstruct에서 필요한 입력 모드(FInputModeUIOnly)를 다시 한번 설정해준다.
 *  이 컨트롤러의 BeginPlay는 그와 별개로 기본 입력 모드를 UI 우호적으로 세팅해두는 안전장치
 *  (혹시 위젯이 이 컨트롤러가 소유하지 않은 다른 경로로 생성/표시되는 경우에도 최소한의 동작 보장).
 *  ACPLobbyGameMode가 PlayerControllerClass로 이 클래스를 기본 지정한다.
 */
UCLASS()
class CP_API ACPLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ACPLobbyPlayerController();

protected:

	virtual void BeginPlay() override;

	/** 디버그용 - 레거시 raw key 입력(EKeys::AnyKey)으로 아무 키나 눌렸는지 감지해서 로그를 남긴다.
	 *  이 로그가 안 뜨면 입력이 이 PlayerController까지도 도달하지 않는 것이므로(예: PIE 창이
	 *  포커스를 못 잡았거나, 실제로는 다른 PlayerController가 쓰이고 있는 경우), UMG 위젯 포커스
	 *  문제가 아니라 그 이전 단계의 문제로 원인을 좁힐 수 있다 */
	virtual void SetupInputComponent() override;

	void HandleAnyKeyPressed_Debug();
};

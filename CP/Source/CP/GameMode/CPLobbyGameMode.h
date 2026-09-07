// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Engine/TimerHandle.h"
#include "CPLobbyGameMode.generated.h"

class UUserWidget;

/** Broadcast whenever a player slot is assigned. PlayerIndex is 0-based (0 = P1, 1 = P2, ...) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPPlayerJoined, int32, PlayerIndex);

/** Broadcast once every requested player slot has been assigned */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPAllPlayersJoined);

/**
 *  로컬 2인 플레이 참가 화면용 GameMode. 아무 입력 장치(키보드/마우스, 게임패드)로든 UCPPlayerJoinWidget이
 *  뜬 뒤 처음 입력한 장치가 1P(PlayerIndex 0), 그 다음 처음 보는 장치가 2P(PlayerIndex 1)가 되도록
 *  "누른 순서"로만 배정한다 - 이미 존재하는 Player 0 컨트롤러를 그대로 재사용하는 것이 아니라, 실제로
 *  가장 먼저 입력을 발생시킨 장치가 PlayerIndex 0을 차지한다. 이 단계에서는 새 로컬 플레이어를
 *  만들거나 입력 장치를 리매핑하지 않고, "이 장치가 몇 번째 플레이어인지"라는 정보만
 *  UCPPlayerRegistrySubsystem에 저장해 다음 레벨로 넘긴다 - 실제 로컬 플레이어 생성/장치 리매핑은
 *  그 정보를 바탕으로 다음(실제 게임플레이) 레벨에서 이루어진다.
 *  NumberOfPlayersToJoin명이 모두 배정되면 LevelLoadDelay초 후 NextLevelName 레벨을 연다.
 *  UCPPlayerJoinWidget이 감지한 입력을 RegisterPlayerInput으로 전달해준다.
 *
 *  BeginPlay에서 StartWidgetClass(보통 UCPPressAnyKeyWidget 상속 WBP)를 자동으로 화면에 띄워주므로,
 *  레벨 블루프린트 등에서 따로 위젯을 생성/배치할 필요 없이 이 GameMode를 상속하는 BP를 만들어
 *  StartWidgetClass/NextLevelName 등만 Class Defaults에서 지정하면 바로 동작한다.
 *  PlayerControllerClass는 기본적으로 ACPLobbyPlayerController로 지정된다.
 */
UCLASS(abstract)
class CP_API ACPLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACPLobbyGameMode();

protected:

	/** 레벨 시작 시 자동으로 화면에 띄울 위젯 (보통 "아무 버튼이나 눌러 시작하세요" 화면인
	 *  UCPPressAnyKeyWidget 상속 WBP). 그 위젯이 자신의 NextWidgetClass를 통해 참가 화면
	 *  (UCPPlayerJoinWidget 상속 WBP)으로 스스로 전환해준다 */
	UPROPERTY(EditDefaultsOnly, Category="Lobby")
	TSubclassOf<UUserWidget> StartWidgetClass;

	virtual void BeginPlay() override;

	/** 참가를 기다릴 플레이어 수 (로컬 2인 플레이면 2) */
	UPROPERTY(EditDefaultsOnly, Category="Lobby", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfPlayersToJoin = 2;

	/** 모든 플레이어 배정이 끝나면 열 다음 레벨 */
	UPROPERTY(EditDefaultsOnly, Category="Lobby")
	FName NextLevelName;

	/** 마지막 플레이어가 배정된 후 다음 레벨을 열기까지 대기하는 시간(초) - 배정 연출을 보여줄 여유 */
	UPROPERTY(EditDefaultsOnly, Category="Lobby", meta = (ClampMin = 0))
	float LevelLoadDelay = 1.0f;

	/** 이미 플레이어 슬롯을 배정받은 입력 장치들. 배열 순서(인덱스)가 곧 그 장치의 PlayerIndex다
	 *  (0번째로 추가된 장치 = PlayerIndex 0 = 1P). PlatformUserId 기준으로 추적하지 않는 이유:
	 *  배정 전에는 아직 연결된 모든 장치가 기본적으로 동일한 UserId를 공유하기 때문에(리매핑 전),
	 *  UserId만으로는 서로 다른 장치를 구분할 수 없다. 이 배열은 GameMode(레벨)가 바뀌면 사라지므로,
	 *  다음 레벨에서도 조회 가능하도록 매 배정마다 UCPPlayerRegistrySubsystem에도 동일한 정보를
	 *  저장해둔다 */
	TArray<FInputDeviceId> ClaimedDeviceIds;

	FTimerHandle LevelLoadTimerHandle;

public:

	/** 플레이어 슬롯이 배정될 때마다 Broadcast */
	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnCPPlayerJoined OnPlayerJoined;

	/** 모든 플레이어 슬롯이 채워지면 Broadcast (레벨 전환 직전) */
	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnCPAllPlayersJoined OnAllPlayersJoined;

	/** UCPPlayerJoinWidget 등이 입력을 감지했을 때 호출. 이미 배정된 장치면 무시하고,
	 *  처음 보는 장치면 지금까지 배정된 장치 수를 그대로 PlayerIndex로 삼아 새 슬롯에 배정한다 -
	 *  즉 이 함수가 가장 먼저 호출된 장치가 PlayerIndex 0(1P)이 된다. 로컬 플레이어를 새로
	 *  만들거나 장치를 리매핑하지는 않고, "이 장치가 몇 번째 플레이어인지"만
	 *  UCPPlayerRegistrySubsystem에 저장한 뒤 OnPlayerJoined를 Broadcast한다.
	 *  FInputDeviceId가 UHT 리플렉션 대상이 아니라 BlueprintCallable로 노출할 수 없어 순수 C++
	 *  함수로 둔다 (호출부인 UCPPlayerJoinWidget도 C++에서 직접 호출) */
	void RegisterPlayerInput(FInputDeviceId DeviceId);

protected:

	/** LevelLoadDelay 경과 후 호출되어 NextLevelName을 연다 */
	void OpenNextLevel();
};

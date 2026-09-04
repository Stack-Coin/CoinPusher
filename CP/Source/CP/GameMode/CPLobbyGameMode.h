// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Engine/TimerHandle.h"
#include "CPLobbyGameMode.generated.h"

/** Broadcast whenever a player slot is assigned. PlayerIndex is 0-based (0 = P1, 1 = P2, ...) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPPlayerJoined, int32, PlayerIndex);

/** Broadcast once every requested player slot has been assigned */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCPAllPlayersJoined);

/**
 *  로컬 2인 플레이 참가 화면용 GameMode. 아무 입력 장치(키보드/마우스, 게임패드)로든 처음
 *  입력한 사람이 1P, 그 다음 처음 보는 장치로 입력한 사람이 2P가 되도록 순서대로 로컬 플레이어를
 *  배정한다. NumberOfPlayersToJoin명이 모두 배정되면 LevelLoadDelay초 후 NextLevelName 레벨을 연다.
 *  UCPPlayerJoinWidget이 감지한 입력을 RegisterPlayerInput으로 전달해준다.
 */
UCLASS(abstract)
class CP_API ACPLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	/** 참가를 기다릴 플레이어 수 (로컬 2인 플레이면 2) */
	UPROPERTY(EditDefaultsOnly, Category="Lobby", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfPlayersToJoin = 2;

	/** 모든 플레이어 배정이 끝나면 열 다음 레벨 */
	UPROPERTY(EditDefaultsOnly, Category="Lobby")
	FName NextLevelName;

	/** 마지막 플레이어가 배정된 후 다음 레벨을 열기까지 대기하는 시간(초) - 배정 연출을 보여줄 여유 */
	UPROPERTY(EditDefaultsOnly, Category="Lobby", meta = (ClampMin = 0))
	float LevelLoadDelay = 1.0f;

	/** 이미 플레이어 슬롯을 배정받은 입력 장치들. UserId가 아니라 DeviceId 기준으로 추적해야
	 *  하는 이유: 배정 전에는 아직 연결된 모든 장치가 기본적으로 Player 0의 UserId를 공유하기
	 *  때문에(리매핑 전), UserId만으로는 서로 다른 장치를 구분할 수 없다 */
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
	 *  처음 보는 장치면 새 플레이어 슬롯(첫 장치는 이미 존재하는 Player 0, 이후로는
	 *  새로 생성한 로컬 플레이어)에 배정한 뒤 OnPlayerJoined를 Broadcast한다.
	 *  FInputDeviceId가 UHT 리플렉션 대상이 아니라 BlueprintCallable로 노출할 수 없어 순수 C++
	 *  함수로 둔다 (호출부인 UCPPlayerJoinWidget도 C++에서 직접 호출) */
	void RegisterPlayerInput(FInputDeviceId DeviceId);

protected:

	/** LevelLoadDelay 경과 후 호출되어 NextLevelName을 연다 */
	void OpenNextLevel();
};

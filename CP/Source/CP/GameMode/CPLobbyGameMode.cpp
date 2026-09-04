// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CPLobbyGameMode.h"
#include "GameMode/CPPlayerRegistrySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

void ACPLobbyGameMode::RegisterPlayerInput(FInputDeviceId DeviceId)
{
	if (!DeviceId.IsValid() || ClaimedDeviceIds.Contains(DeviceId) || ClaimedDeviceIds.Num() >= NumberOfPlayersToJoin)
	{
		return;
	}

	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	UCPPlayerRegistrySubsystem* PlayerRegistry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCPPlayerRegistrySubsystem>() : nullptr;

	if (ClaimedDeviceIds.IsEmpty())
	{
		// 첫 입력은 이미 존재하는 Player 0을 그대로 사용 - 새 플레이어를 만들 필요 없음
		ClaimedDeviceIds.Add(DeviceId);

		const FPlatformUserId FirstPlayerUserId = DeviceMapper.GetUserForInputDevice(DeviceId);
		if (PlayerRegistry)
		{
			// GameInstance에 붙어있어 다음 레벨(OpenLevel 이후)에서도 "PlayerIndex 0 = 이 UserId"를
			// 조회할 수 있다 - 어떤 입력 장치가 어떤 Actor를 조종하는지는 이 정보로 역추적됨
			PlayerRegistry->RegisterJoinedPlayer(FirstPlayerUserId);
		}

		OnPlayerJoined.Broadcast(0);
	}
	else
	{
		// 이후 입력은 새 로컬 플레이어를 만들고, 그 장치만 콕 집어 새 플레이어에게 리매핑
		if (APlayerController* NewPlayerController = UGameplayStatics::CreatePlayer(GetWorld(), -1, true))
		{
			if (ULocalPlayer* NewLocalPlayer = NewPlayerController->GetLocalPlayer())
			{
				const FPlatformUserId OldUserId = DeviceMapper.GetUserForInputDevice(DeviceId);
				const FPlatformUserId NewUserId = NewLocalPlayer->GetPlatformUserId();

				DeviceMapper.Internal_ChangeInputDeviceUserMapping(DeviceId, NewUserId, OldUserId);

				ClaimedDeviceIds.Add(DeviceId);

				if (PlayerRegistry)
				{
					PlayerRegistry->RegisterJoinedPlayer(NewUserId);
				}

				OnPlayerJoined.Broadcast(ClaimedDeviceIds.Num() - 1);
			}
		}
	}

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

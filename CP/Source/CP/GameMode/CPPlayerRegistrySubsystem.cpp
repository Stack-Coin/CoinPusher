// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CPPlayerRegistrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UCPPlayerRegistrySubsystem::RegisterJoinedPlayer(FPlatformUserId PlatformUserId)
{
	JoinedPlayerOrder.AddUnique(PlatformUserId);
}

void UCPPlayerRegistrySubsystem::ResetRegistry()
{
	JoinedPlayerOrder.Reset();
}

FPlatformUserId UCPPlayerRegistrySubsystem::GetPlatformUserIdForPlayerIndex(int32 PlayerIndex) const
{
	return JoinedPlayerOrder.IsValidIndex(PlayerIndex) ? JoinedPlayerOrder[PlayerIndex] : PLATFORMUSERID_NONE;
}

int32 UCPPlayerRegistrySubsystem::GetPlayerIndexForController(APlayerController* PlayerController) const
{
	if (!PlayerController || !PlayerController->GetLocalPlayer())
	{
		return -1;
	}

	return JoinedPlayerOrder.IndexOfByKey(PlayerController->GetLocalPlayer()->GetPlatformUserId());
}

AActor* UCPPlayerRegistrySubsystem::GetControlledActorForPlayerIndex(int32 PlayerIndex) const
{
	const FPlatformUserId TargetUserId = GetPlatformUserIdForPlayerIndex(PlayerIndex);
	if (TargetUserId == PLATFORMUSERID_NONE)
	{
		return nullptr;
	}

	for (ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		if (LocalPlayer && LocalPlayer->GetPlatformUserId() == TargetUserId)
		{
			if (APlayerController* PC = LocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
			{
				return PC->GetPawn();
			}
		}
	}

	return nullptr;
}

AActor* UCPPlayerRegistrySubsystem::GetControlledActorForInputDevice(FInputDeviceId DeviceId) const
{
	const FPlatformUserId TargetUserId = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(DeviceId);

	for (ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		if (LocalPlayer && LocalPlayer->GetPlatformUserId() == TargetUserId)
		{
			if (APlayerController* PC = LocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
			{
				return PC->GetPawn();
			}
		}
	}

	return nullptr;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/CPPlayerRegistrySubsystem.h"

void UCPPlayerRegistrySubsystem::RegisterJoinedPlayer(FInputDeviceId DeviceId)
{
	if (DeviceId.IsValid())
	{
		JoinedPlayerDeviceIds.AddUnique(DeviceId);
	}
}

void UCPPlayerRegistrySubsystem::ResetRegistry()
{
	JoinedPlayerDeviceIds.Reset();
}

int32 UCPPlayerRegistrySubsystem::GetPlayerIndexForInputDevice(FInputDeviceId DeviceId) const
{
	return JoinedPlayerDeviceIds.IndexOfByKey(DeviceId);
}

FInputDeviceId UCPPlayerRegistrySubsystem::GetInputDeviceForPlayerIndex(int32 PlayerIndex) const
{
	return JoinedPlayerDeviceIds.IsValidIndex(PlayerIndex) ? JoinedPlayerDeviceIds[PlayerIndex] : INPUTDEVICEID_NONE;
}

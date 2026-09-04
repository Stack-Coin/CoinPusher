// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPKnockbackInterface.h"
#include "GameFramework/Character.h"

void ApplyCPKnockbackToCharacter(ACharacter* Character, const FVector& Direction, float Distance, float DurationSeconds, float VerticalLaunchStrength)
{
	if (!Character)
	{
		return;
	}

	FVector FlatDirection = Direction;
	FlatDirection.Z = 0.0f;

	if (!FlatDirection.Normalize())
	{
		return;
	}

	const float Speed = DurationSeconds > 0.0f ? (Distance / DurationSeconds) : Distance;
	Character->LaunchCharacter(FlatDirection * Speed + FVector::UpVector * VerticalLaunchStrength, true, true);
}

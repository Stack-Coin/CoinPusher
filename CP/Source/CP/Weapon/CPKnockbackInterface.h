// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPKnockbackInterface.generated.h"

/**
 *  CPKnockbackable
 *  Optional interface for anything that can be pushed back by a weapon hit (melee or projectile).
 *  Weapons never cast to a concrete Enemy class - they cast to this interface instead, so new enemy
 *  types automatically support knockback simply by implementing it.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPKnockbackable : public UInterface
{
	GENERATED_BODY()
};

class ICPKnockbackable
{
	GENERATED_BODY()

public:

	/** Pushes this actor Distance units along Direction. InstigatorActor is whoever caused the knockback */
	UFUNCTION(BlueprintCallable, Category="Knockback")
	virtual void ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor) = 0;
};

/** Shared LaunchCharacter-based physics for an ICPKnockbackable::ApplyKnockback implementation on any
 *  ACharacter - converts Distance into a launch speed via DurationSeconds (Speed = Distance / DurationSeconds,
 *  the same convention as ACPPlayerCharacter's dash/attack lunge) and adds a fixed vertical pop on top, so
 *  every ACharacter implementing this interface doesn't have to duplicate the same physics math */
CP_API void ApplyCPKnockbackToCharacter(class ACharacter* Character, const FVector& Direction, float Distance, float DurationSeconds, float VerticalLaunchStrength);

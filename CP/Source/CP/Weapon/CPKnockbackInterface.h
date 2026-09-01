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

	/** Pushes this actor Distance units along Direction. Instigator is whoever caused the knockback */
	UFUNCTION(BlueprintCallable, Category="Knockback")
	virtual void ApplyKnockback(const FVector& Direction, float Distance, AActor* Instigator) = 0;
};

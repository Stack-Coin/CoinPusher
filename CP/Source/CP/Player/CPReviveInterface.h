// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPReviveInterface.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPReviveProgressProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 *  CPReviveProgressProvider
 *  Exposes a downed character's revive progress so UI (e.g. a floating revive-timer widget) can
 *  read it without depending on a concrete player class.
 */
class ICPReviveProgressProvider
{
	GENERATED_BODY()

public:

	/** Returns true while this character is downed and waiting to be revived */
	UFUNCTION(BlueprintCallable, Category="Revive")
	virtual bool IsDowned() const = 0;

	/** Returns how many seconds of revive time remain, or 0 if no revive is currently in progress */
	UFUNCTION(BlueprintCallable, Category="Revive")
	virtual float GetReviveTimeRemaining() const = 0;

	/** Returns the total time required to fully revive, for UI progress-bar normalization */
	UFUNCTION(BlueprintCallable, Category="Revive")
	virtual float GetReviveDuration() const = 0;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CPWeaponEquipper.generated.h"

class ACPWeaponBase;

/**
 *  CPWeaponEquipper
 *  Implemented by whoever can hold and swap weapons (the player). Weapon pickups and UI only ever
 *  talk to this interface, never to a concrete player class.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCPWeaponEquipper : public UInterface
{
	GENERATED_BODY()
};

class ICPWeaponEquipper
{
	GENERATED_BODY()

public:

	/** Unequips the current weapon (if any) and equips WeaponClass in its place. WeaponClass = None just unequips */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual ACPWeaponBase* EquipWeapon(TSubclassOf<ACPWeaponBase> WeaponClass) = 0;

	/** Returns the currently equipped weapon, or null if unarmed */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual ACPWeaponBase* GetCurrentWeapon() const = 0;
};

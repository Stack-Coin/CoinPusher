// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPWorldItem.h"
#include "CPWeaponPickupItem.generated.h"

class ACPWeaponBase;

/**
 *  ACPWeaponPickupItem
 *  A ACPWorldItem (walk up + press Interact) that equips WeaponClass onto the interactor, replacing
 *  whatever it's currently holding. Talks to the interactor purely through ICPWeaponEquipper, never a
 *  concrete player class. Uses ItemData only to announce the pickup (ICPItemInventory::NotifyItemAcquired,
 *  e.g. for a toast) - it's deliberately never added to the generic owned-items list/UI, since the
 *  currently equipped weapon is shown separately via ICPWeaponEquipper::GetCurrentWeapon().
 */
UCLASS(abstract)
class CP_API ACPWeaponPickupItem : public ACPWorldItem
{
	GENERATED_BODY()

public:

	// ~begin ICPInteractable

	/** Equips WeaponClass onto the interactor if it implements ICPWeaponEquipper, then announces the
	 *  pickup via ICPItemInventory::NotifyItemAcquired if it implements that too. No-ops if unarmed cast fails */
	virtual void Interact(AActor* Interactor) override;

	// ~end ICPInteractable

protected:

	/** Weapon this pickup equips onto the interactor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSubclassOf<ACPWeaponBase> WeaponClass;
};

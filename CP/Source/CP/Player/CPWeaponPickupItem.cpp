// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPWeaponPickupItem.h"
#include "Player/CPWeaponEquipper.h"
#include "Player/CPItemInventory.h"

void ACPWeaponPickupItem::Interact(AActor* Interactor)
{
	if (bCollected)
	{
		return;
	}

	ICPWeaponEquipper* Equipper = Cast<ICPWeaponEquipper>(Interactor);
	if (!Equipper)
	{
		return;
	}

	bCollected = true;

	Equipper->EquipWeapon(WeaponClass);

	// Announces the pickup (e.g. for a toast) without adding it to the generic owned-items list/UI -
	// see ACPWeaponPickupItem's class comment
	if (ICPItemInventory* Inventory = Cast<ICPItemInventory>(Interactor))
	{
		Inventory->NotifyItemAcquired(ItemData);
	}

	Destroy();
}

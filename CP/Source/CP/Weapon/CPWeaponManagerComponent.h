// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CPWeaponManagerComponent.generated.h"

class ACPWeaponBase;

/** Broadcast whenever the currently equipped weapon changes (equip/swap/unequip). NewWeapon is null after an unequip.
 *  Player Animation Blueprints should bind to this instead of polling GetCurrentWeapon() every frame. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPWeaponChanged, ACPWeaponBase*, NewWeapon);

/**
 *  UCPWeaponManagerComponent
 *  Owns weapon equip/swap/unequip and the single CurrentWeapon reference, so a Character doesn't need
 *  its own weapon bookkeeping. The owning Character forwards Attack/EquipWeapon/SwapWeapon/GetCurrentWeapon
 *  calls here instead of touching weapon internals directly.
 */
UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class CP_API UCPWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UCPWeaponManagerComponent();

protected:

	/** Socket on the owning Character's mesh that weapons attach to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FName WeaponHandSocketName = TEXT("WeaponSocket");

	/** Weapon class equipped automatically on BeginPlay. None = unarmed until EquipWeapon is called */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TSubclassOf<ACPWeaponBase> DefaultWeaponClass;

	/** Currently equipped weapon, or null if unarmed */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	TObjectPtr<ACPWeaponBase> CurrentWeapon;

public:

	/** Broadcast whenever CurrentWeapon changes */
	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnCPWeaponChanged OnWeaponChanged;

	/** Gameplay initialization: equips DefaultWeaponClass, if one is set */
	virtual void BeginPlay() override;

	/** Unequips the current weapon (if any), spawns WeaponClass, and equips it to the owner's hand socket. WeaponClass = None just unequips */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	ACPWeaponBase* EquipWeapon(TSubclassOf<ACPWeaponBase> WeaponClass);

	/** Stops the current weapon's attack, detaches it, and destroys it. No-op if unarmed */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void UnequipWeapon();

	/** Equivalent to EquipWeapon - unequips whatever is currently held, then equips NewWeaponClass */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	ACPWeaponBase* SwapWeapon(TSubclassOf<ACPWeaponBase> NewWeaponClass);

	/** Returns the currently equipped weapon, or null if unarmed */
	UFUNCTION(BlueprintPure, Category="Weapon")
	ACPWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	/** Forwards an attack request to the current weapon. Returns false if unarmed or the weapon can't attack right now */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool Attack();
};

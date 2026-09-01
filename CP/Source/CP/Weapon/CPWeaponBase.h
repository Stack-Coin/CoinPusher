// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Weapon/CPWeaponTypes.h"
#include "CPWeaponBase.generated.h"

class ACharacter;
class UStaticMeshComponent;
class UCPWeaponAnimationData;
class ICPStatInterface;

/**
 *  ACPWeaponBase
 *  Common parent for every weapon. Owns the shared attack flow (CanAttack/Attack/StartAttack/
 *  FinishAttack, combo timing via FTimerHandle, and final attack power/speed/interval calculation
 *  against the wielder's stats). Subclasses (ACPMeleeWeapon, ACPRangedWeapon, ...) only implement
 *  ExecuteAttack to decide what a single combo hit actually does.
 */
UCLASS(Abstract, Blueprintable)
class CP_API ACPWeaponBase : public AActor
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACPWeaponBase();

protected:

	/** Actor root, snapped exactly onto the hand socket on Equip. Kept separate from WeaponMesh so each
	 *  weapon Blueprint can freely offset/rotate WeaponMesh (e.g. to fix a mesh authored facing the wrong
	 *  way) without that offset being wiped out by the socket snap - only the root's relative transform
	 *  gets reset by AttachToComponent, a child's does not */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	/** Weapon's visual mesh, parented under WeaponRoot. Collision is disabled - hits are resolved via trace/projectile, not physical mesh collision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** Melee or Ranged. Set per weapon Blueprint */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ECPWeaponType WeaponType = ECPWeaponType::Melee;

	/** Common attack data (power/speed/combo/interval/effect). Tweakable per weapon Blueprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FCPWeaponData WeaponData;

	/** Animation set applied to the wielder while this weapon is equipped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Animation")
	TObjectPtr<UCPWeaponAnimationData> AnimationData;

	/** Floor for GetFinalAttackInterval(), so an extreme AttackSpeed stat can never produce a zero/negative timer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", AdvancedDisplay, meta = (ClampMin = 0.01, Units = "s"))
	float MinAttackInterval = 0.05f;

	/** True from StartAttack until the whole combo string finishes (FinishAttack), not just a single swing */
	bool bIsAttacking = false;

	/** Combo swing currently being executed, in [0, WeaponData.AttackCount) */
	int32 CurrentComboIndex = 0;

	/** Ticks between each combo swing */
	FTimerHandle ComboTimerHandle;

	/** Ticks down the AttackInterval cooldown after the combo string finishes */
	FTimerHandle AttackIntervalTimerHandle;

public:

	/** Attaches this weapon to NewOwner's SocketName and marks NewOwner as its wielder. Called by the weapon manager */
	virtual void EquipTo(ACharacter* NewOwner, FName SocketName);

	/** Stops any in-progress attack and detaches this weapon from its wielder. Called by the weapon manager before destroying/swapping */
	virtual void Unequip();

	/** Returns Melee or Ranged */
	UFUNCTION(BlueprintPure, Category="Weapon")
	ECPWeaponType GetWeaponType() const { return WeaponType; }

	/** Returns this weapon's animation set, or nullptr if none was assigned */
	UFUNCTION(BlueprintPure, Category="Weapon")
	UCPWeaponAnimationData* GetWeaponAnimationData() const { return AnimationData; }

	/** Returns true if a new Attack() call is currently allowed */
	UFUNCTION(BlueprintPure, Category="Weapon")
	virtual bool CanAttack() const;

	/** Entry point for attack input. No-ops if CanAttack() is false */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual void Attack();

	/** Wielder's AttackPower stat (if any) + this weapon's AttackPower */
	UFUNCTION(BlueprintPure, Category="Weapon")
	float GetFinalAttackPower() const;

	/** Wielder's AttackSpeed stat (if any) * this weapon's AttackSpeed, floored above zero */
	UFUNCTION(BlueprintPure, Category="Weapon")
	float GetFinalAttackSpeed() const;

	/** WeaponData.AttackInterval / GetFinalAttackSpeed(), floored at MinAttackInterval */
	UFUNCTION(BlueprintPure, Category="Weapon")
	float GetFinalAttackInterval() const;

protected:

	/** Begins a new combo string: flags the weapon as attacking, plays the attack montage, and runs the first swing */
	virtual void StartAttack();

	/** Ends the combo string and restores CanAttack() to true */
	virtual void FinishAttack();

	/** Runs the current combo swing, then either schedules the next swing (ComboAttackInterval) or FinishAttack (GetFinalAttackInterval) */
	void PerformComboStep();

	/** Runs the actual attack (hit-scan or projectile spawn) for one combo swing. Implemented by subclasses.
	 *  Uses PURE_VIRTUAL (a body that fatal-errors) rather than a true C++ "= 0" - a genuinely pure virtual
	 *  would make this UCLASS(Abstract) type C++-abstract too, which breaks UHT's generated vtable-helper
	 *  constructor. ACPWeaponBase can still never be instantiated directly because of the Abstract specifier. */
	virtual void ExecuteAttack(int32 ComboIndex) PURE_VIRTUAL(ACPWeaponBase::ExecuteAttack, );

	/** Returns the ICPStatInterface implemented by this weapon's owner, or nullptr */
	ICPStatInterface* GetOwnerStatInterface() const;

	/** Returns the ACharacter that owns/wields this weapon, or nullptr */
	ACharacter* GetOwningCharacter() const;

	/** Spawns WeaponData.AttackEffect at Location, if one is assigned */
	void PlayAttackEffect(const FVector& Location) const;
};

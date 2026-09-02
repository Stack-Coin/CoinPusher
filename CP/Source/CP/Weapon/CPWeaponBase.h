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

/** Broadcast right when a combo string starts (true), and right when the attack motion is actually over (false):
 *  when the attack montage finishes/blends out if one is assigned, otherwise as soon as the last swing is
 *  dispatched. Never waits for the post-combo AttackInterval cooldown, since that only rate-limits the next
 *  Attack() call and isn't part of the motion itself. Lets external systems (e.g. the wielder's movement lock)
 *  react to attack motion timing without polling CanAttack() every frame. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCPAttackStateChanged, bool, bIsAttacking);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	/** Weapon's visual mesh, parented under WeaponRoot. Collision is disabled - hits are resolved via trace/projectile, not physical mesh collision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** Melee or Ranged. Set per weapon Blueprint */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ECPWeaponType WeaponType = ECPWeaponType::Melee;

	/** Display name shown by UI (e.g. the currently-equipped-weapon readout). Set per weapon Blueprint */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FText WeaponDisplayName;

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

	/** Aim direction resolved once when Attack() started this combo string (e.g. the mouse-cursor direction at
	 *  click time via ICPAimDirectionProvider) and reused by every swing in the string, so a moving cursor
	 *  during AttackTiming/ComboAttackInterval delays can't change where an already-started attack lands */
	FVector CapturedAttackDirection = FVector::ForwardVector;

	/** True while StartAttack is waiting for the attack montage to actually finish (bound via
	 *  Montage_SetEndDelegate) before broadcasting OnAttackStateChanged(false), instead of PerformComboStep
	 *  releasing it as soon as the last swing is dispatched. False when no montage is assigned to wait for */
	bool bWaitingForMontageEnd = false;

	/** Incremented each StartAttack. Captured by the montage-end callback so a late end/interrupt notification
	 *  from an attack that was since cancelled and restarted can't release the lock for the newer attack */
	int32 AttackSessionId = 0;

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

	/** Returns this weapon's display name, for UI */
	UFUNCTION(BlueprintPure, Category="Weapon")
	FText GetWeaponDisplayName() const { return WeaponDisplayName; }

	/** Returns true if a new Attack() call is currently allowed */
	UFUNCTION(BlueprintPure, Category="Weapon")
	virtual bool CanAttack() const;

	/** Entry point for attack input. No-ops if CanAttack() is false */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual void Attack();

	/** Stops the in-progress combo string immediately (clears its timers, stops the attack montage) and restores
	 *  CanAttack() to true. No-ops if not currently attacking. Lets an external action (e.g. a dash) interrupt a swing */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual void CancelAttack();

	/** Broadcast right when a combo string starts and right when it ends or is cancelled */
	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnCPAttackStateChanged OnAttackStateChanged;

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

	/** Resolves the wielder's current aim direction - ICPAimDirectionProvider if the owner implements it
	 *  (e.g. mouse cursor direction), otherwise the owning character's forward vector. Only called once per
	 *  combo string, from StartAttack - see CapturedAttackDirection */
	FVector ResolveAimDirection() const;

	/** Spawns WeaponData.AttackEffect at Location, if one is assigned */
	void PlayAttackEffect(const FVector& Location) const;
};

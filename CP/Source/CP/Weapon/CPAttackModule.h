// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CPAttackModule.generated.h"

class ACPWeaponBase;
class ACharacter;
class AController;

/** Everything a UCPAttackModule needs to run its follow-up behavior after a melee weapon's base hit is resolved */
struct FCPAttackExecutionContext
{
	/** Weapon that resolved the base hit */
	ACPWeaponBase* Weapon = nullptr;

	/** Character wielding Weapon, or null if unowned */
	ACharacter* OwnerCharacter = nullptr;

	/** Controller credited for any damage this module deals */
	AController* InstigatorController = nullptr;

	/** Actor passed as DamageCauser/knockback instigator (the wielder, or the weapon if unowned) */
	AActor* DamageCauser = nullptr;

	/** World location the base hit was resolved at (the melee shape's center) */
	FVector Origin = FVector::ZeroVector;

	/** Forward direction the base hit was aimed along */
	FVector Direction = FVector::ForwardVector;

	/** Weapon's final attack power for this swing (wielder stat + weapon base), for modules that deal their own damage */
	float AttackPower = 0.0f;

	/** Combo swing index that produced this hit */
	int32 ComboIndex = 0;
};

/**
 *  UCPAttackModule
 *  Optional behavior attached to a melee combo swing (see ACPMeleeWeapon::FCPMeleeComboStepData::PostHitModules),
 *  run once after that swing's own hit-scan damage/knockback has already been applied. Concrete modules (spawn a
 *  projectile, schedule a delayed explosion, ...) are assigned as Instanced sub-objects directly in a weapon
 *  Blueprint, so new attack combinations can be assembled by picking/tuning modules instead of writing new C++.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, Blueprintable)
class CP_API UCPAttackModule : public UObject
{
	GENERATED_BODY()

public:

	/** Runs this module's behavior for one swing. Uses PURE_VIRTUAL rather than a true C++ "= 0" - see
	 *  ACPWeaponBase::ExecuteAttack for why (UCLASS(Abstract) + UHT's generated vtable-helper constructor) */
	virtual void Execute(const FCPAttackExecutionContext& Context) PURE_VIRTUAL(UCPAttackModule::Execute, );
};

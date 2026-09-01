// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPWeaponTypes.generated.h"

class UNiagaraSystem;

/** Broad category a weapon belongs to. Drives which ACPWeaponBase subclass is used */
UENUM(BlueprintType)
enum class ECPWeaponType : uint8
{
	Melee,
	Ranged
};

/** Shape used for a melee weapon's hit-scan attack range */
UENUM(BlueprintType)
enum class ECPMeleeAttackShape : uint8
{
	Sphere,
	Box,
	Capsule
};

/** How a ranged weapon's projectiles are spread out when fired */
UENUM(BlueprintType)
enum class ECPProjectileFirePattern : uint8
{
	/** Fired forward from the owner, fanned out with an angular spread if ProjectileCount > 1 */
	Straight,
	/** Fired outward in a full circle around the owner, evenly spaced by ProjectileCount */
	Radial
};

/** Data every weapon shares, regardless of Melee/Ranged type. Tweakable per weapon Blueprint */
USTRUCT(BlueprintType)
struct FCPWeaponData
{
	GENERATED_BODY()

	/** Base damage dealt by this weapon, added to the wielder's AttackPower stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	float AttackPower = 10.0f;

	/** Attack speed multiplier, combined with the wielder's AttackSpeed stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta = (ClampMin = 0.01))
	float AttackSpeed = 1.0f;

	/** Number of hits performed by a single Attack() call (a combo string) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta = (ClampMin = 1))
	int32 AttackCount = 1;

	/** Delay between each hit within a combo string (only relevant when AttackCount > 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta = (ClampMin = 0, Units = "s"))
	float ComboAttackInterval = 0.3f;

	/** Minimum time after a full combo string finishes before CanAttack() allows another Attack() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta = (ClampMin = 0, Units = "s"))
	float AttackInterval = 0.8f;

	/** Effect played each time this weapon executes an attack (swing/muzzle flash, not a hit-impact effect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TObjectPtr<UNiagaraSystem> AttackEffect;
};

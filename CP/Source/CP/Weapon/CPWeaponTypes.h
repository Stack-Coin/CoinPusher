// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPWeaponTypes.generated.h"

class UNiagaraSystem;
class USoundBase;

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
	Capsule,
	/** Sphere trace additionally filtered to hits within a given angle of the attack direction - a fan/cone-shaped swing */
	Arc
};

/** How a ranged weapon's projectiles are spread out when fired */
UENUM(BlueprintType)
enum class ECPProjectileFirePattern : uint8
{
	/** Fired forward from the owner, fanned out with an angular spread if ProjectileCount > 1 */
	Straight,
	/** Fired outward in a full circle around the owner, evenly spaced by ProjectileCount */
	Radial,
	/** All fired in the same direction, spawned side by side (spread across spawn location, not angle) */
	Parallel,
	/** Randomly scattered within a 3D cone around the aim direction (horizontal AND vertical spread) - a shotgun-style spread */
	Cone
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

	/** Effect played each time this weapon executes an attack (swing/muzzle flash, not a hit-impact effect).
	 *  Spawned oriented to face the attack direction and scaled by AttackEffectScale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TObjectPtr<UNiagaraSystem> AttackEffect;

	/** Uniform/non-uniform scale applied to AttackEffect when it's spawned. Tune per weapon instead of
	 *  editing the Niagara System's own internal size for a quick per-weapon size adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FVector AttackEffectScale = FVector(1.0f, 1.0f, 1.0f);

	/** Added on top of the attack direction when AttackEffect is spawned, so an effect authored facing a
	 *  different axis (or needing a consistent tilt/twist) can be corrected per weapon without editing the
	 *  Niagara System itself */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FRotator AttackEffectRotationOffset = FRotator::ZeroRotator;

	/** Added to the swing/muzzle origin before AttackEffect is spawned, relative to the attack direction
	 *  (X = forward along the attack direction, Y = right, Z = up) - same convention as
	 *  ACPMeleeWeapon::FCPMeleeComboStepData::PostHitModuleOffset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FVector AttackEffectLocationOffset = FVector::ZeroVector;

	/** Sound played each time this weapon executes an attack (swing/muzzle blast, not a hit-impact sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TObjectPtr<USoundBase> AttackSound;

	/** Volume multiplier applied to AttackSound when it's played. Tune per weapon instead of editing the
	 *  sound asset itself for a quick per-weapon loudness adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta = (ClampMin = 0))
	float AttackSoundVolume = 1.0f;

	/** Added to the swing/muzzle origin before AttackSound is played, relative to the attack direction
	 *  (X = forward along the attack direction, Y = right, Z = up) - same convention as AttackEffectLocationOffset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FVector AttackSoundLocationOffset = FVector::ZeroVector;
};

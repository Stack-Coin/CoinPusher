// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Weapon/CPWeaponBase.h"
#include "CPMeleeWeapon.generated.h"

/**
 *  ACPMeleeWeapon
 *  Melee weapon that resolves each combo swing with a hit-scan shape trace (Sphere/Box/Capsule) in
 *  front of the wielder, instead of physical mesh collision. ExecuteMeleeHit is kept separate from
 *  ExecuteAttack so a future Animation Notify can trigger the hit directly instead of relying on AttackTiming.
 */
UCLASS(Blueprintable)
class CP_API ACPMeleeWeapon : public ACPWeaponBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACPMeleeWeapon();

protected:

	/** Shape used for the attack hit-scan */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee")
	ECPMeleeAttackShape AttackRangeShape = ECPMeleeAttackShape::Box;

	/** Interpreted per AttackRangeShape - Sphere: X = Radius. Box: XYZ = full extent (width/length/height). Capsule: X = Radius, Y = HalfHeight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (Units = "cm"))
	FVector AttackRangeSize = FVector(150.0f, 100.0f, 100.0f);

	/** Distance the attack range's center is offset in front of the wielder */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (Units = "cm"))
	float AttackRangeOffset = 75.0f;

	/** Delay from the start of a swing to the actual hit-scan, to line up with the attack animation's impact frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (ClampMin = 0, Units = "s"))
	float AttackTiming = 0.2f;

	/** Distance a hit target is knocked back, if it implements ICPKnockbackable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 200.0f;

	/** If true, draws the attack hit-scan shape for debugging */
	UPROPERTY(EditAnywhere, Category="Melee|Debug")
	bool bDrawDebugAttackShape = false;

	/** Delays ExecuteMeleeHit by AttackTiming after a swing starts */
	FTimerHandle MeleeHitTimerHandle;

protected:

	// ~begin ACPWeaponBase

	/** Schedules ExecuteMeleeHit after AttackTiming (or runs it immediately if AttackTiming is 0) */
	virtual void ExecuteAttack(int32 ComboIndex) override;

	// ~end ACPWeaponBase

	/** Runs the hit-scan shape trace and applies damage/knockback/effects to everything it hits. Public entry point for a future Animation Notify */
	UFUNCTION(BlueprintCallable, Category="Melee")
	virtual void ExecuteMeleeHit();

	/** Forward direction the attack is aimed along - the wielder's forward vector, or this actor's if unowned */
	FVector GetAttackDirection() const;

	/** World location the attack shape trace is centered on */
	FVector GetAttackOrigin() const;
};

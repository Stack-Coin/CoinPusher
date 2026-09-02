// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Weapon/CPWeaponBase.h"
#include "CPMeleeWeapon.generated.h"

class UCPAttackModule;

/**
 *  Per-combo-swing hit-scan configuration, plus optional follow-up modules (see UCPAttackModule) run once
 *  the swing's own base hit has been resolved. ACPMeleeWeapon::ComboSteps holds one of these per combo swing.
 */
USTRUCT(BlueprintType)
struct FCPMeleeComboStepData
{
	GENERATED_BODY()

	/** Shape used for this swing's hit-scan. Arc additionally filters hits within ArcAngle of the attack direction - set RangeOffset to 0 to pivot it on the wielder's own location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee")
	ECPMeleeAttackShape Shape = ECPMeleeAttackShape::Box;

	/** Interpreted per Shape - Sphere/Arc: X = Radius. Box: XYZ = full extent (width/length/height). Capsule: X = Radius, Y = HalfHeight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (Units = "cm"))
	FVector ShapeSize = FVector(150.0f, 100.0f, 100.0f);

	/** Full arc angle in degrees (e.g. 120 = 60 degrees to either side of the attack direction). Only used when Shape == Arc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (ClampMin = 0, ClampMax = 360, Units = "deg", EditCondition = "Shape == ECPMeleeAttackShape::Arc"))
	float ArcAngle = 120.0f;

	/** Distance the shape's center is offset in front of the wielder */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (Units = "cm"))
	float RangeOffset = 75.0f;

	/** Delay from the start of this swing to its hit-scan, to line up with the attack animation's impact frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (ClampMin = 0, Units = "s"))
	float AttackTiming = 0.2f;

	/** Distance a hit target is knocked back by this swing itself, if it implements ICPKnockbackable. A PostHitModules entry (e.g. an explosion) applies its own knockback separately */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 200.0f;

	/** Extra behavior run once after this swing's base hit is resolved (e.g. spawn a projectile, schedule a delayed explosion). Add module instances directly - see UCPAttackModule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", Instanced)
	TArray<TObjectPtr<UCPAttackModule>> PostHitModules;

	/** Extra offset applied on top of the hit-scan origin before it's handed to PostHitModules, so a follow-up
	 *  effect (e.g. where an explosion lands, where a projectile spawns) can be positioned independently of the
	 *  hit-scan shape itself. X = forward along the attack direction, Y = right of it, Z = world up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee", meta = (Units = "cm"))
	FVector PostHitModuleOffset = FVector::ZeroVector;
};

/**
 *  ACPMeleeWeapon
 *  Melee weapon that resolves each combo swing with a hit-scan shape trace (Sphere/Box/Capsule/Arc) in
 *  front of the wielder, instead of physical mesh collision. Each swing's shape/range/timing/knockback and
 *  optional follow-up modules are configured per entry in ComboSteps. ExecuteMeleeHit is kept separate from
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

	/** One entry per combo swing - WeaponData.AttackCount should match this array's length. A ComboIndex beyond the array clamps to the last entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee")
	TArray<FCPMeleeComboStepData> ComboSteps;

	/** If true, draws the current swing's hit-scan shape for debugging */
	UPROPERTY(EditAnywhere, Category="Melee|Debug")
	bool bDrawDebugAttackShape = false;

	/** Delays ExecuteMeleeHit by the current swing's AttackTiming after that swing starts */
	FTimerHandle MeleeHitTimerHandle;

	/** ComboSteps index ExecuteMeleeHit should resolve against - set by ExecuteAttack right before scheduling/running the hit */
	int32 PendingHitComboIndex = 0;

protected:

	// ~begin ACPWeaponBase

	/** Schedules ExecuteMeleeHit after the swing's AttackTiming (or runs it immediately if AttackTiming is 0) */
	virtual void ExecuteAttack(int32 ComboIndex) override;

	/** Also clears the pending ExecuteMeleeHit timer, on top of ACPWeaponBase's own combo/interval timers */
	virtual void CancelAttack() override;

	// ~end ACPWeaponBase

	/** Runs the hit-scan shape trace for ComboSteps[PendingHitComboIndex] and applies damage/knockback/effects/modules to everything it hits. Public entry point for a future Animation Notify */
	UFUNCTION(BlueprintCallable, Category="Melee")
	virtual void ExecuteMeleeHit();

	/** Forward direction the attack is aimed along - the wielder's forward vector, or this actor's if unowned */
	FVector GetAttackDirection() const;

	/** World location the attack shape trace is centered on, offset by InRangeOffset in front of the wielder */
	FVector GetAttackOrigin(float InRangeOffset) const;

	/** Returns ComboSteps[ComboIndex], clamped to the last valid entry (or a default-constructed entry if ComboSteps is empty) */
	const FCPMeleeComboStepData& GetComboStepData(int32 ComboIndex) const;

	/** True if HitLocation falls within FullArcAngleDegrees of Direction, measured horizontally (XY plane) from Origin */
	static bool IsWithinArc(const FVector& Origin, const FVector& Direction, const FVector& HitLocation, float FullArcAngleDegrees);
};

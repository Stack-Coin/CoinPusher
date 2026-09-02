// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPAttackModule.h"
#include "CPDelayedExplosionModule.generated.h"

class ACPDelayedExplosion;

/**
 *  UCPDelayedExplosionModule
 *  Attack module that spawns an ACPDelayedExplosion at Context.Origin, which detonates after a delay (radial
 *  damage + outward knockback). Used for a "strike a point, then it blows up" follow-up, e.g. a downward slash
 *  that explodes a second later. See ACPMeleeWeapon::FCPMeleeComboStepData::PostHitModuleOffset to move the
 *  explosion point independently of the swing's own hit-scan shape.
 */
UCLASS(EditInlineNew, meta = (DisplayName = "Delayed Explosion"))
class CP_API UCPDelayedExplosionModule : public UCPAttackModule
{
	GENERATED_BODY()

public:

	// ~begin UCPAttackModule

	virtual void Execute(const FCPAttackExecutionContext& Context) override;

	// ~end UCPAttackModule

protected:

	/** Explosion actor class spawned by this module. Defaults to ACPDelayedExplosion itself if left unset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion")
	TSubclassOf<ACPDelayedExplosion> ExplosionClass;

	/** Delay, in seconds, between this swing landing and the explosion detonating */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion", meta = (ClampMin = 0, Units = "s"))
	float ExplosionDelay = 1.0f;

	/** Radius of the explosion's damage/knockback check */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion", meta = (ClampMin = 0, Units = "cm"))
	float ExplosionRadius = 300.0f;

	/** Distance hit targets are knocked back, outward from the explosion's center, if they implement ICPKnockbackable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion", meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 400.0f;
};

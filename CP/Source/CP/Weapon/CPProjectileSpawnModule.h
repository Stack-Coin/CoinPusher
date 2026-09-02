// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPAttackModule.h"
#include "CPProjectileSpawnModule.generated.h"

class ACPProjectile;

/**
 *  UCPProjectileSpawnModule
 *  Attack module that spawns a projectile from the melee swing's hit origin, traveling along the swing's
 *  attack direction - used to add a ranged follow-up to a melee combo (e.g. a sword-wave fired on the final
 *  hit of a combo string). Reuses ACPProjectile as-is: a wide/slow projectile Blueprint (bigger collision
 *  sphere) approximates a cone-shaped wave without needing a dedicated shape.
 */
UCLASS(EditInlineNew, meta = (DisplayName = "Spawn Projectile"))
class CP_API UCPProjectileSpawnModule : public UCPAttackModule
{
	GENERATED_BODY()

public:

	// ~begin UCPAttackModule

	virtual void Execute(const FCPAttackExecutionContext& Context) override;

	// ~end UCPAttackModule

protected:

	/** Projectile actor spawned by this module, at Context.Origin - see ACPMeleeWeapon::FCPMeleeComboStepData::PostHitModuleOffset to reposition it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	TSubclassOf<ACPProjectile> ProjectileClass;
};

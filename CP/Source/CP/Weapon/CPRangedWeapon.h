// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/CPWeaponBase.h"
#include "CPRangedWeapon.generated.h"

class ACPProjectile;

/**
 *  ACPRangedWeapon
 *  Ranged weapon that fires ProjectileClass on each combo swing, according to FirePattern: straight ahead
 *  (with an optional angular fan when ProjectileCount > 1), radially around the wielder, side by side
 *  (Parallel), or randomly scattered within a 3D cone (Cone, a shotgun-style spread).
 */
UCLASS(Blueprintable)
class CP_API ACPRangedWeapon : public ACPWeaponBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACPRangedWeapon();

protected:

	/** Projectile actor spawned by this weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged")
	TSubclassOf<ACPProjectile> ProjectileClass;

	/** Number of projectiles spawned per swing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged", meta = (ClampMin = 1))
	int32 ProjectileCount = 1;

	/** Straight: fan out in front of the wielder. Radial: spread evenly in a full circle around the wielder */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged")
	ECPProjectileFirePattern FirePattern = ECPProjectileFirePattern::Straight;

	/** Total angular spread (degrees) the ProjectileCount projectiles fan out across. Only used by the Straight pattern when ProjectileCount > 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged", meta = (ClampMin = 0, Units = "deg"))
	float SpreadAngle = 15.0f;

	/** Distance between adjacent projectiles' spawn points, perpendicular to the fire direction. Only used by the Parallel pattern when ProjectileCount > 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged", meta = (ClampMin = 0, Units = "cm"))
	float ParallelSpacing = 50.0f;

	/** Full angle (degrees) of the 3D cone each projectile is randomly scattered within, around the aim direction. Only used by the Cone pattern */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged", meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float ConeAngle = 30.0f;

	/** Socket on WeaponMesh projectiles spawn from. Falls back to this weapon's location if the socket doesn't exist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Additional world-space offset applied to the spawn location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ranged")
	FVector ProjectileSpawnOffset = FVector::ZeroVector;

protected:

	// ~begin ACPWeaponBase

	/** Spawns ProjectileCount projectiles according to FirePattern */
	virtual void ExecuteAttack(int32 ComboIndex) override;

	// ~end ACPWeaponBase

	/** Resolves MuzzleSocketName on WeaponMesh, or this actor's location if the socket isn't found */
	FVector GetMuzzleLocation() const;

	/** Base fire direction - the wielder's forward vector, or this actor's if unowned */
	FVector GetBaseFireDirection() const;

	/** Computes one fire direction per projectile according to FirePattern/ProjectileCount/SpreadAngle. Not used by the Parallel pattern - see ComputeParallelSpawnLocations */
	TArray<FVector> ComputeFireDirections(const FVector& BaseDirection) const;

	/** Computes one spawn location per projectile for the Parallel pattern - all fired along BaseDirection, spread sideways around BaseLocation by ParallelSpacing */
	TArray<FVector> ComputeParallelSpawnLocations(const FVector& BaseLocation, const FVector& BaseDirection) const;

	/** Spawns a single projectile at Location, facing Direction, and hands it its damage/instigator info */
	void SpawnProjectile(const FVector& Location, const FVector& Direction);
};

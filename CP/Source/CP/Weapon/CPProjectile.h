// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

/**
 *  ACPProjectile
 *  Common projectile fired by ranged weapons. Moves via UProjectileMovementComponent (no Tick), and
 *  destroys itself once it travels Range (SetLifeSpan, computed from Range/ProjectileSpeed) instead of
 *  tracking distance every frame. Pawns overlap (so CanPierce can let the projectile continue through
 *  them); world geometry blocks it and always destroys it.
 */
UCLASS(Blueprintable)
class CP_API ACPProjectile : public AActor
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACPProjectile();

protected:

	/** Collision root. Overlaps Pawns (so CanPierce can apply), blocks world geometry */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionComp;

	/** Optional visual mesh, purely cosmetic - collision is handled by CollisionComp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	/** Drives movement, including optional homing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** If true, the projectile continues after hitting a target instead of being destroyed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	bool bCanPierce = false;

	/** Maximum travel distance before the projectile destroys itself */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta = (ClampMin = 0, Units = "cm"))
	float Range = 2000.0f;

	/** If true, homes in on the target set via SetHomingTarget (if any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	bool bIsHoming = false;

	/** Travel speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta = (ClampMin = 0, Units = "cm/s"))
	float ProjectileSpeed = 2000.0f;

	/** Distance a hit target is knocked back, if it implements ICPKnockbackable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 150.0f;

	/** Effect played at the impact location when this projectile hits something */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	TObjectPtr<UNiagaraSystem> HitEffect;

	/** Damage dealt on hit. Set by the firing weapon via InitializeProjectile - not designer-editable per instance */
	float DamageAmount = 0.0f;

	/** Controller credited for damage dealt by this projectile */
	TWeakObjectPtr<AController> InstigatorController;

	/** Actor passed as DamageCauser/knockback instigator (the wielder, or the weapon if unowned) */
	TWeakObjectPtr<AActor> DamageCauserActor;

	/** Actors already hit by this projectile, so a piercing projectile can't damage the same target twice */
	TSet<TWeakObjectPtr<AActor>> HitActors;

public:

	/** Gameplay initialization: applies ProjectileSpeed to the movement component and sets the range-based lifespan */
	virtual void BeginPlay() override;

	/** Called by the firing weapon right after spawn to set up damage and instigator info */
	void InitializeProjectile(float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser);

	/** Sets (or clears, if Target is null) the homing target. Only takes effect if bIsHoming is true */
	UFUNCTION(BlueprintCallable, Category="Projectile")
	void SetHomingTarget(AActor* Target);

protected:

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Shared hit handling: applies damage/knockback/hit effect once per actor, then destroys unless CanPierce */
	void ProcessHit(AActor* OtherActor, const FVector& HitLocation);
};

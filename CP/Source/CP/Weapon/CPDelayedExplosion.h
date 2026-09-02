// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "CPDelayedExplosion.generated.h"

class UNiagaraSystem;

/**
 *  ACPDelayedExplosion
 *  Lightweight actor spawned at a strike location that detonates after a delay: deals radial damage and
 *  pushes everything within Radius outward from its own location (knockback direction points away from the
 *  explosion's center, not the original attacker's forward vector), then destroys itself. Owns its own timer
 *  so the explosion isn't cancelled if the weapon that spawned it starts another swing in the meantime.
 */
UCLASS(Blueprintable)
class CP_API ACPDelayedExplosion : public AActor
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACPDelayedExplosion();

protected:

	/** Effect played at the explosion location on detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

	/** If true, draws the detonation's damage/knockback radius for debugging */
	UPROPERTY(EditAnywhere, Category="Explosion|Debug")
	bool bDrawDebugExplosionRadius = false;

	/** Radius of the detonation's damage/knockback check. Set by InitializeExplosion */
	float Radius = 300.0f;

	/** Distance hit targets are knocked back, outward from the explosion's center. Set by InitializeExplosion */
	float KnockbackDistance = 400.0f;

	/** Damage dealt on detonation. Set by InitializeExplosion */
	float DamageAmount = 0.0f;

	/** Controller credited for damage dealt by this explosion */
	TWeakObjectPtr<AController> InstigatorController;

	/** Actor passed as DamageCauser/knockback instigator (the wielder, or the weapon if unowned) */
	TWeakObjectPtr<AActor> DamageCauserActor;

	/** Ticks down to Detonate() */
	FTimerHandle DetonateTimerHandle;

public:

	/** Called right after spawn to schedule detonation and set up damage/knockback parameters. Detonates immediately if Delay <= 0 */
	void InitializeExplosion(float Delay, float InRadius, float InKnockbackDistance, float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser);

protected:

	/** Runs the radial damage/knockback check, plays ExplosionEffect, then destroys this actor */
	UFUNCTION()
	void Detonate();
};

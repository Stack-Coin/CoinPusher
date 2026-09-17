// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Debug/CPDebugTypes.h"
#include "CPProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UMaterialInstanceDynamic;

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

	/** Persistent visual effect attached to the projectile for its entire flight (e.g. a trail/glow) - assign
	 *  the Niagara System directly on this component in the projectile Blueprint. Distinct from HitEffect,
	 *  which is a one-shot burst spawned only at the impact location. Its System's Emitter(s) need Local
	 *  Space enabled for the effect to actually track this projectile's motion/scale rather than being left
	 *  behind at its spawn transform - see ProjectileEffectLocationOffset/RotationOffset/Scale below */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNiagaraComponent> ProjectileEffectComponent;

	/** Relative location applied to ProjectileEffectComponent at spawn, relative to this projectile's own
	 *  facing (X = forward, Y = right, Z = up) - leave the component's own Relative Location at (0,0,0) in
	 *  the Blueprint and tune this instead, for the same offset convention used by HitEffect/LaunchSound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector ProjectileEffectLocationOffset = FVector::ZeroVector;

	/** Relative rotation applied to ProjectileEffectComponent at spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FRotator ProjectileEffectRotationOffset = FRotator::ZeroRotator;

	/** Relative scale applied to ProjectileEffectComponent at spawn. Only visually affects the effect if its
	 *  Niagara System's Emitter(s) have Local Space enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector ProjectileEffectScale = FVector(1.0f, 1.0f, 1.0f);

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

	/** Uniform/non-uniform scale applied to HitEffect when it's spawned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector HitEffectScale = FVector(1.0f, 1.0f, 1.0f);

	/** Added on top of the projectile's travel direction at the moment of impact when HitEffect is spawned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FRotator HitEffectRotationOffset = FRotator::ZeroRotator;

	/** Added to the impact location before HitEffect is spawned, relative to the projectile's travel
	 *  direction at the moment of impact (X = forward along travel, Y = right, Z = up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector HitEffectLocationOffset = FVector::ZeroVector;

	/** Sound played once, at spawn, when this projectile is launched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	TObjectPtr<USoundBase> LaunchSound;

	/** Added to the spawn location before LaunchSound is played, relative to this projectile's fire direction
	 *  (X = forward along travel, Y = right, Z = up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector LaunchSoundLocationOffset = FVector::ZeroVector;

	/** If true, redraws CollisionComp's sphere at its current location on a short repeating timer for debugging.
	 *  Uses a timer rather than Tick, since CollisionComp itself moves every frame via ProjectileMovementComponent */
	UPROPERTY(EditAnywhere, Category="Projectile|Debug")
	bool bDrawDebugCollision = false;

	/** Redraws CollisionComp's sphere at its current location. Bound to DebugDrawTimerHandle when bDrawDebugCollision is true */
	FTimerHandle DebugDrawTimerHandle;

	/** How long the projectile takes to fade to transparent (ProjectileMesh's Opacity/FadeOpacityParameterName
	 *  material parameter, 1 -> 0) before actually being destroyed - on a hit, a piercing hit's final Range
	 *  timeout, or the Range timeout itself. 0 = destroyed instantly, no fade (old behavior) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Fade Out", meta = (ClampMin = 0, Units = "s"))
	float FadeOutDuration = 0.3f;

	/** Scalar parameter name BeginFadeOutAndDestroy drives from 1 (opaque) to 0 (transparent) on ProjectileMesh's
	 *  per-instance dynamic material(s). ProjectileMesh's material needs a Translucent/Masked blend mode and a
	 *  parameter with this name wired into its opacity - set up in the material/material instance, not here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Fade Out")
	FName FadeOpacityParameterName = TEXT("Opacity");

	/** ProjectileMesh's materials, converted to dynamic instances the first time BeginFadeOutAndDestroy runs */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FadeMaterialMIDs;

	/** True from the first BeginFadeOutAndDestroy call until the projectile is actually destroyed - guards
	 *  against a second hit (or the Range timeout) restarting the fade while it's already in progress */
	bool bIsFadingOut = false;

	/** Counts up from 0 to FadeOutDuration on FadeOutTimerHandle */
	float FadeOutElapsedTime = 0.0f;

	/** Ticks FadeOutElapsedTime/updates FadeMaterialMIDs while fading out, then destroys the projectile */
	FTimerHandle FadeOutTimerHandle;

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
	void InitializeProjectile(float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser, float InRangeMultiplier = 1.0f);

	/** Sets (or clears, if Target is null) the homing target. Only takes effect if bIsHoming is true */
	UFUNCTION(BlueprintCallable, Category="Projectile")
	void SetHomingTarget(AActor* Target);

protected:

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Shared hit handling: applies damage/knockback/hit effect once per actor, then destroys (see
	 *  BeginFadeOutAndDestroy) unless CanPierce */
	void ProcessHit(AActor* OtherActor, const FVector& HitLocation);

	/** Draws CollisionComp's sphere at its current location. Called on DebugDrawTimerHandle while bDrawDebugCollision is true */
	void DrawDebugCollisionShape() const;

	/** Bound to UCPDebugCollisionSubsystem::OnCollisionVisibilityChanged. Starts/stops DebugDrawTimerHandle
	 *  to match the F1 debug widget's PlayerWeapon checkbox */
	UFUNCTION()
	void HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible);

	/** Stops movement/collision/the trail effect and, if FadeOutDuration > 0, fades ProjectileMesh's dynamic
	 *  materials from opaque to transparent over that duration before destroying the projectile - instead of
	 *  disappearing instantly. Called instead of Destroy() from every place this projectile used to destroy
	 *  itself (a blocking hit, a non-piercing overlap hit, and the Range-based lifespan timeout below).
	 *  No-ops if a fade is already in progress */
	void BeginFadeOutAndDestroy();

	/** Bound to FadeOutTimerHandle while fading out - advances FadeOutElapsedTime, updates FadeMaterialMIDs'
	 *  FadeOpacityParameterName, and actually destroys the projectile once FadeOutElapsedTime reaches FadeOutDuration */
	void TickFadeOut();

	/** Overridden so the Range-based lifespan (SetLifeSpan in BeginPlay) fades out too instead of the engine's
	 *  default instant Destroy() */
	virtual void LifeSpanExpired() override;
};

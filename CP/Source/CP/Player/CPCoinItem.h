// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Player/CPInteractable.h"
#include "Debug/CPDebugTypes.h"
#include "CPCoinItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UCPDebugCollisionShapeComponent;

/**
 *  Simple pickup that grants coins to the overlapping pawn's ICPCoinWallet the instant it overlaps a
 *  pawn. No key press is needed - the same ICPInteractable::Interact() call every interactable uses
 *  is simply triggered by the coin itself on overlap, instead of by a player key press.
 */
UCLASS(abstract)
class CP_API ACPCoinItem : public AActor, public ICPInteractable
{
	GENERATED_BODY()

	/** Collision used to detect the player. Root component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Purely visual, no collision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* CoinMesh;

	/** Draws CollisionSphere's wireframe while the F1 debug widget's ItemPickup checkbox is on */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCPDebugCollisionShapeComponent* DebugPickupShape;

protected:

	/** How many coins this coin grants when collected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin", meta = (ClampMin = 0))
	int32 CoinValue = 1;

	/** Simple visual spin speed in degrees/second, purely cosmetic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	float RotationSpeed = 90.0f;

	/** Collision stays off for this long after spawning, so a coin dropped right where a monster just died
	 *  isn't instantly vacuumed up by the player standing on top of it - it's visible for a beat first */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin", meta = (ClampMin = 0, Units = "s"))
	float PickupDelay = 0.3f;

	/** True once this coin has already been collected, to guard against duplicate overlaps */
	bool bCollected = false;

	/** Re-enables CollisionSphere after PickupDelay. Started from BeginPlay */
	FTimerHandle PickupDelayTimerHandle;

public:

	/** Constructor */
	ACPCoinItem();

protected:

	/** Disables CollisionSphere until PickupDelay elapses (see EnableCollection) */
	virtual void BeginPlay() override;

	/** Cosmetic spin */
	virtual void Tick(float DeltaTime) override;

	/** Turns CollisionSphere back on and immediately re-checks overlaps, so a pawn already standing on
	 *  this coin when the delay ends is collected right away instead of needing to step off and back on */
	void EnableCollection();

	/** Bound to CollisionSphere's OnComponentBeginOverlap */
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	// ~begin ICPInteractable
	virtual void Interact(AActor* Interactor) override;
	virtual FText GetInteractableDisplayName() const override;
	// ~end ICPInteractable
};

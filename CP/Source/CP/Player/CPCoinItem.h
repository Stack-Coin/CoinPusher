// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/CPInteractable.h"
#include "CPCoinItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  Simple pickup that grants coins the instant it overlaps something that implements ICPCoinWallet.
 *  No key press is needed - the same ICPInteractable::Interact() call every interactable uses
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

protected:

	/** How many coins this coin grants when collected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin", meta = (ClampMin = 0))
	int32 CoinValue = 1;

	/** Simple visual spin speed in degrees/second, purely cosmetic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	float RotationSpeed = 90.0f;

	/** True once this coin has already been collected, to guard against duplicate overlaps */
	bool bCollected = false;

public:

	/** Constructor */
	ACPCoinItem();

protected:

	/** Cosmetic spin */
	virtual void Tick(float DeltaTime) override;

	/** Bound to CollisionSphere's OnComponentBeginOverlap */
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	// ~begin ICPInteractable
	virtual void Interact(AActor* Interactor) override;
	virtual FText GetInteractableDisplayName() const override;
	// ~end ICPInteractable
};

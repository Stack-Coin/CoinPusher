// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherItem.h"
#include "CPItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  A physics-simulated prize item. Dispensed by ACPDispenser like a coin, but when collected
 *  by ACPDropZone it records its ItemCode instead of incrementing the coin count.
 */
UCLASS(abstract)
class CP_API ACPItem : public AActor, public ICPCoinPusherItem
{
	GENERATED_BODY()

	/** Physics collision shape and RootComponent. A sphere so the item rolls naturally */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Visual sphere mesh, no collision - purely cosmetic */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:

	/** Constructor */
	ACPItem();

protected:

	/** Identifies which item this is */
	UPROPERTY(EditAnywhere, Category="Item")
	FName ItemId;

	/** If true, this item has already been collected and is awaiting destruction */
	bool bCollected = false;

public:

	/** Returns this item's identifying code */
	UFUNCTION(BlueprintPure, Category="Item")
	FName GetItemId() const { return ItemId; }

	/** Called by ACPDropZone once it has already recorded this item (RecordCollectedItem) - plays the BP
	 *  collection effect and destroys this actor. Returns false (and does nothing else) if this item was
	 *  already collected, so a duplicate call can't double up */
	UFUNCTION(BlueprintCallable, Category="Item")
	bool Collect();

	// ~begin ICPCoinPusherItem interface

	/** Calls Collect() (DropZone already read GetItemCode() and recorded this item directly at the
	 *  overlap, so this no longer reports anything back to it) */
	virtual void OnDroppedInZone(ACPDropZone* DropZone) override;

	// ~end ICPCoinPusherItem interface

protected:

	/** Passes control to BP to play effects on collection */
	UFUNCTION(BlueprintImplementableEvent, Category="Item", meta = (DisplayName = "On Collected"))
	void BP_OnCollected();

public:

	/** Returns the CollisionSphere subobject */
	FORCEINLINE USphereComponent* GetCollisionSphere() const { return CollisionSphere; }

	/** Returns the Mesh subobject */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
};

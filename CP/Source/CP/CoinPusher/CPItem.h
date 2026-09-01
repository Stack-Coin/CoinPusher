// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPCoinPusherItem.h"
#include "CPItem.generated.h"

class UStaticMeshComponent;

/**
 *  A physics-simulated prize item. Dispensed by ACPDispenser like a coin, but when collected
 *  by ACPDropZone it records its ItemCode instead of incrementing the coin count.
 */
UCLASS(abstract)
class CP_API ACPItem : public AActor, public ICPCoinPusherItem
{
	GENERATED_BODY()

	/** Item mesh, simulates physics */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:

	/** Constructor */
	ACPItem();

protected:

	/** Identifies which item this is */
	UPROPERTY(EditAnywhere, Category="Item")
	FName ItemCode;

	/** If true, this item has already been collected and is awaiting destruction */
	bool bCollected = false;

public:

	/** Returns this item's identifying code */
	UFUNCTION(BlueprintPure, Category="Item")
	FName GetItemCode() const { return ItemCode; }

	// ~begin ICPCoinPusherItem interface

	/** Records the item code with the drop zone, then removes itself */
	virtual void OnDroppedInZone(ACPDropZone* DropZone) override;

	// ~end ICPCoinPusherItem interface

protected:

	/** Passes control to BP to play effects on collection */
	UFUNCTION(BlueprintImplementableEvent, Category="Item", meta = (DisplayName = "On Collected"))
	void BP_OnCollected();

public:

	/** Returns the Mesh subobject */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
};

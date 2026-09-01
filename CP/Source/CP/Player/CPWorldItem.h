// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/CPInteractable.h"
#include "Player/CPItemTypes.h"
#include "CPWorldItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  A pickup an ICPInteractor (the player) must walk up to and press the Interact key to collect.
 *  Registers/unregisters itself with any overlapping ICPInteractor purely through interfaces -
 *  it never depends on a concrete player class.
 */
UCLASS(abstract)
class CP_API ACPWorldItem : public AActor, public ICPInteractable
{
	GENERATED_BODY()

	/** Detects nearby interactors. Root component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

	/** Purely visual, no collision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ItemMesh;

protected:

	/** Data for the item this pickup grants */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FCPItemData ItemData;

	/** True once this item has already been collected, to guard against duplicate interactions */
	bool bCollected = false;

public:

	/** Constructor */
	ACPWorldItem();

protected:

	/** Bound to InteractionRange's OnComponentBeginOverlap */
	UFUNCTION()
	void OnInteractionRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Bound to InteractionRange's OnComponentEndOverlap */
	UFUNCTION()
	void OnInteractionRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:

	// ~begin ICPInteractable
	virtual void Interact(AActor* Interactor) override;
	virtual FText GetInteractableDisplayName() const override;
	// ~end ICPInteractable
};

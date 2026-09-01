// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Player/CPInteractable.h"
#include "CPInput.generated.h"

class UStaticMeshComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputInteracted, AActor*, Interactor);

UCLASS(abstract)
class CP_API ACPInput : public AActor, public ICPInteractable
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Overlap-only detection volume, since Mesh itself blocks (BlockAllDynamic) and won't fire overlaps */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

public:
	ACPInput();

public:

	UPROPERTY(BlueprintAssignable, Category="Input")
	FOnInputInteracted OnInteracted;

	//ICPInteractable
	virtual void Interact(AActor* Interactor) override;

	/** Bound to CollisionSphere's OnComponentBeginOverlap. Registers this as an interact target while the player is in range */
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Bound to CollisionSphere's OnComponentEndOverlap. Unregisters this once the player leaves range */
	UFUNCTION()
	void OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:


	UFUNCTION(BlueprintImplementableEvent, Category="Input", meta = (DisplayName = "On Interacted"))
	void BP_OnInteracted(AActor* Interactor);

public:

	//������Ʈ ��ȯ
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
};

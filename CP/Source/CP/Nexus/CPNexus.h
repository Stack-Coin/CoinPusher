// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Player/CPInteractable.h"
#include "CPNexus.generated.h"

class UStaticMeshComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNexusInteracted, AActor*, Interactor);

UCLASS()
class CP_API ACPNexus : public AActor, public ICPInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACPNexus();

public:
	//ICPInteractable
	virtual void Interact(AActor* Interactor) override;

	/** Bound to CollisionSphere's OnComponentBeginOverlap. Registers this as an interact target while the player is in range */
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Bound to CollisionSphere's OnComponentEndOverlap. Unregisters this once the player leaves range */
	UFUNCTION()
	void OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	void Dead();
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Input", meta = (DisplayName = "On Interacted"))
	void BP_OnInteracted(AActor* Interactor);

public:
	UPROPERTY(BlueprintAssignable, Category = "Input")
	FOnNexusInteracted OnInteracted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collider")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

protected:
	float MaxHp = 100.f;
	float CurrentHp = 100.f;
};

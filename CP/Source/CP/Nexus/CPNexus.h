// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Player/CPInteractable.h"
#include "CPNexus.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UCPUserWidget_NexusHpBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNexusInteracted, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNexusHpChanged, ACPNexus*, Nexus, float, NewCurrentHp);

UCLASS()
class CP_API ACPNexus : public AActor, public ICPInteractable
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ACPNexus();

	virtual void Tick(float DeltaTime) override;

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
	float GetMaxHp() const { return MaxHp; }
	float GetCurrentHp() const { return CurrentHp; }

protected:
	virtual void BeginPlay() override;
	void UpdateHpBar();

	UFUNCTION(BlueprintImplementableEvent, Category = "Input", meta = (DisplayName = "On Interacted"))
	void BP_OnInteracted(AActor* Interactor);

public:
	UPROPERTY(BlueprintAssignable, Category = "Input")
	FOnNexusInteracted OnInteracted;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnNexusHpChanged OnNexusHpChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collider")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HpBar;

	UPROPERTY(Transient)
	TObjectPtr<UCPUserWidget_NexusHpBar> HpBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HealAmount = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float RespawnOffsetDistance = 1000.f;

	float MaxHp = 100.f;
	float CurrentHp = 100.f;
};

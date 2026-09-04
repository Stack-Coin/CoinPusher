// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPGoddess.generated.h"

class USphereComponent;
class UWidgetComponent;
class UCPUserWidget_NexusHpBar;
class ACPNexus;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGoddessDead);

UCLASS()
class CP_API ACPGoddess : public AActor
{
	GENERATED_BODY()
	
public:	
	ACPGoddess();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION()
	void OnNexusHpChanged(ACPNexus* Nexus, float NewCurrentHp);

public:
	float GetMaxHp() const { return MaxHp; }
	float GetCurrentHp() const { return CurrentHp; }

public:
	void UpdateHpBar();
	void Dead();

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnGoddessDead OnGoddessDead;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collider")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HpBar;

	UPROPERTY(Transient)
	TObjectPtr<UCPUserWidget_NexusHpBar> HpBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HealAmount = 30.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACPNexus>> Nexuses;

private:
	UPROPERTY(VisibleAnywhere, Category = "Stat")
	float MaxHp = 200.f;

	UPROPERTY(VisibleAnywhere, Category = "Stat")
	float CurrentHp = 200.f;

	int8 bIsDead : 1 = false;
};

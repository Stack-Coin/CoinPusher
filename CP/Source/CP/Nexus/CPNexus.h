// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPNexus.generated.h"

UCLASS()
class CP_API ACPNexus : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACPNexus();

public:
	void Dead();
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collider")
	TObjectPtr<UStaticMeshComponent> Mesh;

	float MaxHp = 100.f;
	float CurrentHp = 100.f;
};

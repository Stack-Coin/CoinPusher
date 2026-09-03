// Fill out your copyright notice in the Description page of Project Settings.


#include "Nexus/CPNexus.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ACPNexus::ACPNexus()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

void ACPNexus::Dead() 
{
	UE_LOG(LogTemp, Warning, TEXT("Nexus: %f"), CurrentHp);
}

float ACPNexus::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHp -= DamageAmount;

	if (CurrentHp <= 0) 
	{
		Dead();
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("Nexus: %f"), CurrentHp);
	}

	return DamageAmount;
}


// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPCoin.h"
#include "Components/StaticMeshComponent.h"

ACPCoin::ACPCoin()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));
	Mesh->SetSimulatePhysics(true);
	Mesh->bNavigationRelevant = false;
}

void ACPCoin::Launch(const FVector& LaunchVelocity)
{
	Mesh->SetPhysicsLinearVelocity(LaunchVelocity);
}

void ACPCoin::Collect()
{
	if (!bCollected)
	{
		bCollected = true;

		BP_OnCollected();

		Destroy();
	}
}

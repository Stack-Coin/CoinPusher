// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPInput.h"
#include "Components/StaticMeshComponent.h"

ACPInput::ACPInput()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));

	Mesh->bNavigationRelevant = false;
}

void ACPInput::Interact(AActor* Interactor)
{

	OnInteracted.Broadcast(Interactor);

	BP_OnInteracted(Interactor);
}

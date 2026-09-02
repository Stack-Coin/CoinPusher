// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPItem.h"
#include "CPDropZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ACPItem::ACPItem()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the collision sphere, root component
	RootComponent = CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(20.0f);
	CollisionSphere->SetCollisionProfileName(FName("BlockAllDynamic"));

	// enable physics so the item reacts to the Pusher and gravity, same as a coin
	CollisionSphere->SetSimulatePhysics(true);

	// disable navigation relevance so items don't affect NavMesh generation
	CollisionSphere->bNavigationRelevant = false;

	// create the visual mesh, purely cosmetic
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->bNavigationRelevant = false;
}

void ACPItem::OnDroppedInZone(ACPDropZone* DropZone)
{
	// only process this once
	if (bCollected)
	{
		return;
	}

	bCollected = true;

	if (DropZone)
	{
		DropZone->RecordCollectedItem(ItemCode);
	}

	// call the BP handler to play effects, etc.
	BP_OnCollected();

	Destroy();
}

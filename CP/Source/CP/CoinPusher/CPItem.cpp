// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPItem.h"
#include "CPDropZone.h"
#include "Components/StaticMeshComponent.h"

ACPItem::ACPItem()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the mesh
	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	// set the collision properties
	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));

	// enable physics so the item reacts to the Pusher and gravity, same as a coin
	Mesh->SetSimulatePhysics(true);

	// disable navigation relevance so items don't affect NavMesh generation
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

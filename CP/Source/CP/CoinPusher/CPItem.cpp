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

bool ACPItem::Collect()
{
	// only process this once
	if (bCollected)
	{
		return false;
	}

	bCollected = true;

	// call the BP handler to play effects, etc.
	BP_OnCollected();

	Destroy();

	return true;
}

void ACPItem::OnDroppedInZone(ACPDropZone* DropZone)
{
	// DropZone already read GetItemCode() and called RecordCollectedItem() directly at the overlap - just
	// clean ourselves up, no need to report back to it
	Collect();
}

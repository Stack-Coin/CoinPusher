// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPWorldItem.h"
#include "Player/CPInteractor.h"
#include "Player/CPItemInventory.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ACPWorldItem::ACPWorldItem()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	SetRootComponent(InteractionRange);
	InteractionRange->InitSphereRadius(150.0f);
	InteractionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(InteractionRange);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &ACPWorldItem::OnInteractionRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddDynamic(this, &ACPWorldItem::OnInteractionRangeEndOverlap);
}

void ACPWorldItem::OnInteractionRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected || !OtherActor)
	{
		return;
	}

	if (ICPInteractor* Interactor = Cast<ICPInteractor>(OtherActor))
	{
		Interactor->RegisterInteractable(this);
	}
}

void ACPWorldItem::OnInteractionRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	if (ICPInteractor* Interactor = Cast<ICPInteractor>(OtherActor))
	{
		Interactor->UnregisterInteractable(this);
	}
}

void ACPWorldItem::Interact(AActor* Interactor)
{
	if (bCollected)
	{
		return;
	}

	ICPItemInventory* Inventory = Cast<ICPItemInventory>(Interactor);
	if (!Inventory)
	{
		return;
	}

	bCollected = true;

	Inventory->AddOwnedItem(ItemData);

	Destroy();
}

FText ACPWorldItem::GetInteractableDisplayName() const
{
	return ItemData.ItemName;
}

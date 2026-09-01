// Copyright Epic Games, Inc. All Rights Reserved.


#include "CPInput.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Player/CPInteractor.h"

ACPInput::ACPInput()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));

	Mesh->bNavigationRelevant = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(Mesh);
	CollisionSphere->InitSphereRadius(150.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACPInput::OnCollisionSphereBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ACPInput::OnCollisionSphereEndOverlap);
}

void ACPInput::Interact(AActor* Interactor)
{

	OnInteracted.Broadcast(Interactor);

	BP_OnInteracted(Interactor);
}

void ACPInput::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	if (ICPInteractor* Interactor = Cast<ICPInteractor>(OtherActor))
	{
		Interactor->RegisterInteractable(this);
	}
}

void ACPInput::OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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

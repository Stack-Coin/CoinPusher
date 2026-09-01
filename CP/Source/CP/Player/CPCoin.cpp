// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPCoin.h"
#include "Player/CPCoinWallet.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ACPCoin::ACPCoin()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(50.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(CollisionSphere);
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACPCoin::OnCollisionSphereBeginOverlap);
}

void ACPCoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
}

void ACPCoin::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected || !OtherActor)
	{
		return;
	}

	if (Cast<ICPCoinWallet>(OtherActor))
	{
		Interact(OtherActor);
	}
}

void ACPCoin::Interact(AActor* Interactor)
{
	if (bCollected)
	{
		return;
	}

	ICPCoinWallet* Wallet = Cast<ICPCoinWallet>(Interactor);
	if (!Wallet)
	{
		return;
	}

	bCollected = true;

	Wallet->AddCoin(CoinValue);

	Destroy();
}

FText ACPCoin::GetInteractableDisplayName() const
{
	return FText::FromString(TEXT("Coin"));
}

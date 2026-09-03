// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPCoinItem.h"
#include "Player/CPCoinWallet.h"
#include "Player/CPGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"

ACPCoinItem::ACPCoinItem()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(50.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(CollisionSphere);
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACPCoinItem::OnCollisionSphereBeginOverlap);
}

void ACPCoinItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
}

void ACPCoinItem::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Coins are picked up by pawns (the player) - the wallet itself now lives on ACPGameMode (team-shared),
	// not on whatever overlapped, so this just gates on "a pawn touched it" instead of an ICPCoinWallet cast
	if (bCollected || !Cast<APawn>(OtherActor))
	{
		return;
	}

	Interact(OtherActor);
}

void ACPCoinItem::Interact(AActor* Interactor)
{
	if (bCollected)
	{
		return;
	}

	ICPCoinWallet* Wallet = GetWorld() ? Cast<ICPCoinWallet>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (!Wallet)
	{
		return;
	}

	bCollected = true;

	Wallet->AddCoin(CoinValue);

	Destroy();
}

FText ACPCoinItem::GetInteractableDisplayName() const
{
	return FText::FromString(TEXT("Coin"));
}

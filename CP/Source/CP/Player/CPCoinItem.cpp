// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPCoinItem.h"
#include "Player/CPCoinWallet.h"
#include "Player/CPInteractor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/CPDebugCollisionShapeComponent.h"
#include "TimerManager.h"

ACPCoinItem::ACPCoinItem()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(50.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	DebugPickupShape = CreateDefaultSubobject<UCPDebugCollisionShapeComponent>(TEXT("DebugPickupShape"));
	DebugPickupShape->Category = ECPDebugCollisionCategory::ItemPickup;
	DebugPickupShape->ShapeColor = FColor::Cyan;
	DebugPickupShape->SetTargetComponent(CollisionSphere);

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(CollisionSphere);
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACPCoinItem::OnCollisionSphereBeginOverlap);
}

void ACPCoinItem::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().SetTimer(PickupDelayTimerHandle, this, &ACPCoinItem::EnableCollection, PickupDelay, false);
}

void ACPCoinItem::EnableCollection()
{
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->UpdateOverlaps();
}

void ACPCoinItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
}

void ACPCoinItem::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Gate on ICPInteractor (implemented only by the player) rather than "any Pawn touched it" - a monster's
	// own corpse is a Pawn too and can easily still be sitting right on top of a coin it just dropped
	if (bCollected || !Cast<ICPInteractor>(OtherActor))
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

	ICPCoinWallet* Wallet = Cast<ICPCoinWallet>(Interactor);
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

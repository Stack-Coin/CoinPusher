// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPDelayedExplosion.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

ACPDelayedExplosion::ACPDelayedExplosion()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ACPDelayedExplosion::InitializeExplosion(float Delay, float InRadius, float InKnockbackDistance, float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser)
{
	Radius = InRadius;
	KnockbackDistance = InKnockbackDistance;
	DamageAmount = InDamageAmount;
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;

	if (bDrawDebugExplosionRadius)
	{
		// This actor doesn't move, so a single sphere lasting the full countdown previews where/how big the
		// detonation will be, rather than only seeing it flash at the moment it actually goes off
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 16, FColor::Orange, false, FMath::Max(Delay, 0.0f), 0, 1.0f);
	}

	if (Delay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DetonateTimerHandle, this, &ACPDelayedExplosion::Detonate, Delay, false);
	}
	else
	{
		Detonate();
	}
}

void ACPDelayedExplosion::Detonate()
{
	const FVector Origin = GetActorLocation();

	TArray<AActor*> ActorsToIgnore;
	if (AActor* DamageCauser = DamageCauserActor.Get())
	{
		ActorsToIgnore.Add(DamageCauser);
	}

	if (bDrawDebugExplosionRadius)
	{
		// A short, distinct flash exactly at detonation, on top of InitializeExplosion's longer countdown
		// preview - makes the actual "it's exploding now" moment unambiguous instead of relying on
		// SphereTraceMulti's own generic trace-debug color/duration
		DrawDebugSphere(GetWorld(), Origin, Radius, 16, FColor::Red, false, 0.5f, 0, 2.0f);
	}

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(
		this, Origin, Origin, Radius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
		EDrawDebugTrace::None, HitResults, true);

	// A single actor can report multiple hit results, so only process each unique target once per detonation
	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.Add(HitActor);

		UGameplayStatics::ApplyDamage(HitActor, DamageAmount, InstigatorController.Get(), DamageCauserActor.Get(), nullptr);

		if (ICPKnockbackable* Knockbackable = Cast<ICPKnockbackable>(HitActor))
		{
			FVector Direction = (HitActor->GetActorLocation() - Origin).GetSafeNormal2D();
			if (Direction.IsNearlyZero())
			{
				Direction = FVector::ForwardVector;
			}
			Knockbackable->ApplyKnockback(Direction, KnockbackDistance, DamageCauserActor.Get());
		}
	}

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect, Origin);
	}

	Destroy();
}

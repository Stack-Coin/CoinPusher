// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPProjectile.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

namespace
{
	// Refresh rate for ACPProjectile's debug collision draw - a moving shape redrawn this often reads as continuous
	constexpr float DebugDrawInterval = 0.05f;
}

ACPProjectile::ACPProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(15.0f);
	CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
	// Pawns overlap instead of block, so CanPierce can let the projectile continue moving through them
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->OnComponentHit.AddDynamic(this, &ACPProjectile::OnProjectileHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ACPProjectile::OnProjectileOverlap);
	RootComponent = CollisionComp;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ACPProjectile::BeginPlay()
{
	Super::BeginPlay();

	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->bIsHomingProjectile = bIsHoming && ProjectileMovement->HomingTargetComponent.IsValid();
	ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileSpeed;

	if (ProjectileSpeed > 0.0f)
	{
		SetLifeSpan(Range / ProjectileSpeed);
	}

	if (bDrawDebugCollision)
	{
		DrawDebugCollisionShape();
		GetWorldTimerManager().SetTimer(DebugDrawTimerHandle, this, &ACPProjectile::DrawDebugCollisionShape, DebugDrawInterval, true);
	}
}

void ACPProjectile::InitializeProjectile(float InDamageAmount, AController* InInstigatorController, AActor* InDamageCauser)
{
	DamageAmount = InDamageAmount;
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;
}

void ACPProjectile::SetHomingTarget(AActor* Target)
{
	if (!ProjectileMovement)
	{
		return;
	}

	if (Target)
	{
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
		ProjectileMovement->bIsHomingProjectile = bIsHoming;
	}
	else
	{
		ProjectileMovement->HomingTargetComponent = nullptr;
		ProjectileMovement->bIsHomingProjectile = false;
	}
}

void ACPProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ProcessHit(OtherActor, bFromSweep ? FVector(SweepResult.Location) : GetActorLocation());
}

void ACPProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// A blocking hit only ever happens against world geometry (Pawns are set to overlap above), so it always stops the projectile
	ProcessHit(OtherActor, Hit.Location);
	Destroy();
}

void ACPProjectile::ProcessHit(AActor* OtherActor, const FVector& HitLocation)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	if (HitActors.Contains(OtherActor))
	{
		return;
	}
	HitActors.Add(OtherActor);

	UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, InstigatorController.Get(), DamageCauserActor.Get(), nullptr);

	if (ICPKnockbackable* Knockbackable = Cast<ICPKnockbackable>(OtherActor))
	{
		const FVector Direction = ProjectileMovement ? ProjectileMovement->Velocity.GetSafeNormal() : GetActorForwardVector();
		Knockbackable->ApplyKnockback(Direction, KnockbackDistance, DamageCauserActor.Get());
	}

	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, HitLocation);
	}

	if (!bCanPierce)
	{
		Destroy();
	}
}

void ACPProjectile::DrawDebugCollisionShape() const
{
	if (!CollisionComp)
	{
		return;
	}

	DrawDebugSphere(GetWorld(), CollisionComp->GetComponentLocation(), CollisionComp->GetScaledSphereRadius(), 16, FColor::Cyan, false, DebugDrawInterval * 1.5f, 0, 1.0f);
}

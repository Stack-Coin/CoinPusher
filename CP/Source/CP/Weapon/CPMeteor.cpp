#include "Weapon/CPMeteor.h"
#include "Weapon/CPMeteorGroundZone.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Debug/CPDebugCollisionSubsystem.h"

namespace
{
	constexpr float MeteorDebugDrawInterval = 0.1f;
}

ACPMeteor::ACPMeteor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeteorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeteorMesh"));
	SetRootComponent(MeteorMesh);
	MeteorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeteorMesh->SetGenerateOverlapEvents(false);

	MeteorMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MeteorMovement"));
	MeteorMovement->SetUpdatedComponent(MeteorMesh);
	MeteorMovement->ProjectileGravityScale = 0.0f;
	MeteorMovement->bRotationFollowsVelocity = false;
}

void ACPMeteor::BeginPlay()
{
	Super::BeginPlay();

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &ACPMeteor::HandleDebugCollisionVisibilityChanged);
		bDrawDebugImpactRadius = Subsystem->IsCategoryVisible(ECPDebugCollisionCategory::PlayerWeapon);
	}
}

void ACPMeteor::InitializeMeteor(const FVector& InLandingLocation, float FallSpeed, float InImpactRadius, float InImpactDamage, float InImpactKnockbackDistance, TSubclassOf<ACPMeteorGroundZone> InGroundZoneClass, float InGroundZoneDuration, float InGroundZoneDamagePerTick, float InGroundZoneTickInterval, float InGroundZoneRadius, AController* InInstigatorController, AActor* InDamageCauser)
{
	LandingLocation = InLandingLocation;
	ImpactRadius = InImpactRadius;
	ImpactDamage = InImpactDamage;
	ImpactKnockbackDistance = InImpactKnockbackDistance;
	GroundZoneClass = InGroundZoneClass;
	GroundZoneDuration = InGroundZoneDuration;
	GroundZoneDamagePerTick = InGroundZoneDamagePerTick;
	GroundZoneTickInterval = InGroundZoneTickInterval;
	GroundZoneRadius = InGroundZoneRadius;
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;

	const float SafeFallSpeed = FMath::Max(FallSpeed, 1.0f);
	MeteorMovement->Velocity = FVector::DownVector * SafeFallSpeed;
	MeteorMovement->InitialSpeed = SafeFallSpeed;
	MeteorMovement->MaxSpeed = SafeFallSpeed;

	const float FallHeight = GetActorLocation().Z - LandingLocation.Z;
	const float FallDuration = FallHeight > 0.0f ? FallHeight / SafeFallSpeed : 0.0f;

	SetDebugDrawEnabled(bDrawDebugImpactRadius);

	if (FallDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ImpactTimerHandle, this, &ACPMeteor::Impact, FallDuration, false);
	}
	else
	{
		Impact();
	}
}

void ACPMeteor::Impact()
{
	SetActorLocation(LandingLocation);

	if (bDrawDebugImpactRadius)
	{
		DrawDebugSphere(GetWorld(), LandingLocation, ImpactRadius, 16, FColor::Red, false, 0.5f, 0, 2.0f);
	}

	TArray<AActor*> ActorsToIgnore;
	if (AActor* DamageCauser = DamageCauserActor.Get())
	{
		ActorsToIgnore.Add(DamageCauser);
	}

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(
		this, LandingLocation, LandingLocation, ImpactRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
		EDrawDebugTrace::None, HitResults, true);

	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.Add(HitActor);

		UGameplayStatics::ApplyDamage(HitActor, ImpactDamage, InstigatorController.Get(), DamageCauserActor.Get(), nullptr);

		if (ICPKnockbackable* Knockbackable = Cast<ICPKnockbackable>(HitActor))
		{
			FVector Direction = (HitActor->GetActorLocation() - LandingLocation).GetSafeNormal2D();
			if (Direction.IsNearlyZero())
			{
				Direction = FVector::ForwardVector;
			}
			Knockbackable->ApplyKnockback(Direction, ImpactKnockbackDistance, DamageCauserActor.Get());
		}
	}

	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, LandingLocation);
	}

	if (GroundZoneClass && GroundZoneDuration > 0.0f)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = DamageCauserActor.Get();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (ACPMeteorGroundZone* GroundZone = GetWorld()->SpawnActor<ACPMeteorGroundZone>(GroundZoneClass, LandingLocation, FRotator::ZeroRotator, SpawnParams))
		{
			GroundZone->InitializeZone(GroundZoneRadius, GroundZoneDamagePerTick, GroundZoneTickInterval, GroundZoneDuration, InstigatorController.Get(), DamageCauserActor.Get());
		}
	}

	Destroy();
}

void ACPMeteor::HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (Category != ECPDebugCollisionCategory::PlayerWeapon)
	{
		return;
	}

	SetDebugDrawEnabled(bVisible);
}

void ACPMeteor::SetDebugDrawEnabled(bool bEnabled)
{
	bDrawDebugImpactRadius = bEnabled;
	GetWorldTimerManager().ClearTimer(DebugDrawTimerHandle);

	if (bEnabled)
	{
		DrawDebugImpactShape();
		GetWorldTimerManager().SetTimer(DebugDrawTimerHandle, this, &ACPMeteor::DrawDebugImpactShape, MeteorDebugDrawInterval, true);
	}
}

void ACPMeteor::DrawDebugImpactShape() const
{
	DrawDebugSphere(GetWorld(), LandingLocation, ImpactRadius, 16, FColor::Orange, false, MeteorDebugDrawInterval * 1.5f, 0, 1.0f);
}

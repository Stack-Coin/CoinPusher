#include "Weapon/CPOrbitingCrescent.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Debug/CPDebugCollisionSubsystem.h"

ACPOrbitingCrescent::ACPOrbitingCrescent()
{
	PrimaryActorTick.bCanEverTick = true;

	CrescentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrescentMesh"));
	SetRootComponent(CrescentMesh);
	CrescentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrescentMesh->SetGenerateOverlapEvents(false);
}

void ACPOrbitingCrescent::BeginPlay()
{
	Super::BeginPlay();

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &ACPOrbitingCrescent::HandleDebugCollisionVisibilityChanged);
		bDrawDebugHitRadius = Subsystem->IsCategoryVisible(ECPDebugCollisionCategory::PlayerWeapon);
	}
}

void ACPOrbitingCrescent::HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (Category != ECPDebugCollisionCategory::PlayerWeapon)
	{
		return;
	}

	bDrawDebugHitRadius = bVisible;
}

void ACPOrbitingCrescent::InitializeCrescent(ACharacter* InOwnerCharacter, float InOrbitRadius, float InOrbitSpeedDegPerSec, float InSelfSpinSpeedDegPerSec, float InHitRadius, float InVerticalOffset, float InDuration, float InDamage, float InKnockbackDistance, float InDamageTickInterval, AController* InInstigatorController, AActor* InDamageCauser)
{
	OrbitOwner = InOwnerCharacter;
	OrbitRadius = InOrbitRadius;
	OrbitSpeedDegPerSec = InOrbitSpeedDegPerSec;
	SelfSpinSpeedDegPerSec = InSelfSpinSpeedDegPerSec;
	HitRadius = InHitRadius;
	VerticalOffset = InVerticalOffset;
	Damage = InDamage;
	KnockbackDistance = InKnockbackDistance;
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;

	GetWorldTimerManager().SetTimer(DamageTickTimerHandle, this, &ACPOrbitingCrescent::ApplyPulseDamage, FMath::Max(InDamageTickInterval, 0.01f), true);

	SetLifeSpan(FMath::Max(InDuration, 0.01f));
}

void ACPOrbitingCrescent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACharacter* OrbitCenterCharacter = OrbitOwner.Get();
	if (!OrbitCenterCharacter)
	{
		Destroy();
		return;
	}

	CurrentOrbitAngleDegrees += OrbitSpeedDegPerSec * DeltaTime;
	CurrentSelfSpinDegrees += SelfSpinSpeedDegPerSec * DeltaTime;

	const float OrbitRadians = FMath::DegreesToRadians(CurrentOrbitAngleDegrees);
	const FVector Offset = FVector(FMath::Cos(OrbitRadians), FMath::Sin(OrbitRadians), 0.0f) * OrbitRadius + FVector(0.0f, 0.0f, VerticalOffset);

	SetActorLocation(OrbitCenterCharacter->GetActorLocation() + Offset);
	SetActorRotation(FRotator(0.0f, CurrentSelfSpinDegrees, 0.0f));

	if (bDrawDebugHitRadius)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), HitRadius, 12, FColor::Cyan, false, 0.05f, 0, 1.0f);
	}
}

void ACPOrbitingCrescent::ApplyPulseDamage()
{
	const FVector Origin = GetActorLocation();

	if (bDrawDebugHitRadius)
	{
		DrawDebugSphere(GetWorld(), Origin, HitRadius, 12, FColor::Purple, false, 0.2f, 0, 1.0f);
	}

	TArray<AActor*> ActorsToIgnore;
	if (AActor* DamageCauser = DamageCauserActor.Get())
	{
		ActorsToIgnore.Add(DamageCauser);
	}
	if (ACharacter* OrbitCenterCharacter = OrbitOwner.Get())
	{
		ActorsToIgnore.Add(OrbitCenterCharacter);
	}

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(
		this, Origin, Origin, HitRadius,
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

		UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController.Get(), DamageCauserActor.Get(), nullptr);

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

	if (HitEffect && HitActors.Num() > 0)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, Origin);
	}
}

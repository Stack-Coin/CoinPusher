#include "Weapon/CPMeteorGroundZone.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Debug/CPDebugCollisionSubsystem.h"

namespace
{
	constexpr float GroundZoneDebugDrawInterval = 0.1f;
}

ACPMeteorGroundZone::ACPMeteorGroundZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZoneEffect"));
	SetRootComponent(ZoneEffect);
	ZoneEffect->SetAutoActivate(true);
}

void ACPMeteorGroundZone::BeginPlay()
{
	Super::BeginPlay();

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &ACPMeteorGroundZone::HandleDebugCollisionVisibilityChanged);
		bDrawDebugZoneRadius = Subsystem->IsCategoryVisible(ECPDebugCollisionCategory::PlayerWeapon);
	}
}

void ACPMeteorGroundZone::InitializeZone(float InZoneRadius, float InDamagePerTick, float InTickInterval, float InDuration, AController* InInstigatorController, AActor* InDamageCauser)
{
	ZoneRadius = InZoneRadius;
	DamagePerTick = InDamagePerTick;
	TickInterval = FMath::Max(InTickInterval, 0.01f);
	InstigatorController = InInstigatorController;
	DamageCauserActor = InDamageCauser;

	SetDebugDrawEnabled(bDrawDebugZoneRadius);

	GetWorldTimerManager().SetTimer(DamageTickTimerHandle, this, &ACPMeteorGroundZone::ApplyZoneDamage, TickInterval, true);

	SetLifeSpan(FMath::Max(InDuration, 0.01f));
}

void ACPMeteorGroundZone::ApplyZoneDamage()
{
	const FVector Origin = GetActorLocation();

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(
		this, Origin, Origin, ZoneRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, TArray<AActor*>(),
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

		UGameplayStatics::ApplyDamage(HitActor, DamagePerTick, InstigatorController.Get(), DamageCauserActor.Get(), nullptr);
	}
}

void ACPMeteorGroundZone::HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (Category != ECPDebugCollisionCategory::PlayerWeapon)
	{
		return;
	}

	SetDebugDrawEnabled(bVisible);
}

void ACPMeteorGroundZone::SetDebugDrawEnabled(bool bEnabled)
{
	bDrawDebugZoneRadius = bEnabled;
	GetWorldTimerManager().ClearTimer(DebugDrawTimerHandle);

	if (bEnabled)
	{
		DrawDebugZoneShape();
		GetWorldTimerManager().SetTimer(DebugDrawTimerHandle, this, &ACPMeteorGroundZone::DrawDebugZoneShape, GroundZoneDebugDrawInterval, true);
	}
}

void ACPMeteorGroundZone::DrawDebugZoneShape() const
{
	DrawDebugSphere(GetWorld(), GetActorLocation(), ZoneRadius, 16, FColor::Orange, false, GroundZoneDebugDrawInterval * 1.5f, 0, 1.0f);
}

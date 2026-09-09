#include "Weapon/CPMeteorPassiveSkillModule.h"
#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPMeteor.h"
#include "Weapon/CPMeteorGroundZone.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void UCPMeteorPassiveSkillModule::Activate(const FCPPassiveSkillActivationContext& Context)
{
	UWorld* World = Context.Weapon ? Context.Weapon->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPMeteorPassiveSkillModule::Activate - no World (Context.Weapon is %s)"), Context.Weapon ? TEXT("valid") : TEXT("null"));
		return;
	}

	TSubclassOf<ACPMeteor> ClassToSpawn = MeteorClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACPMeteor::StaticClass();
	}

	const FVector LandingLocation = ComputeLandingLocation(Context.Origin, World);
	const FVector SpawnLocation = LandingLocation + FVector(0.0f, 0.0f, FallHeight);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Context.DamageCauser;
	SpawnParams.Instigator = Context.OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPMeteor* Meteor = World->SpawnActor<ACPMeteor>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!Meteor)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPMeteorPassiveSkillModule::Activate - SpawnActor failed for class '%s' at %s"), *GetNameSafe(ClassToSpawn.Get()), *SpawnLocation.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UCPMeteorPassiveSkillModule::Activate - spawned '%s', landing at %s (fall height %.0f, radius %.0f)"), *GetNameSafe(Meteor), *LandingLocation.ToString(), FallHeight, ImpactRadius);

	Meteor->InitializeMeteor(LandingLocation, FallSpeed, ImpactRadius, ImpactDamage, ImpactKnockbackDistance, GroundZoneClass, GroundZoneDuration, GroundZoneDamagePerTick, GroundZoneTickInterval, GroundZoneRadius, Context.InstigatorController, Context.DamageCauser);
}

FVector UCPMeteorPassiveSkillModule::ComputeLandingLocation(const FVector& Center, UWorld* World) const
{
	const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
	const float RandomDistance = FMath::FRandRange(0.0f, DropRadius);
	const FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(RandomAngle)), FMath::Sin(FMath::DegreesToRadians(RandomAngle)), 0.0f) * RandomDistance;

	const FVector TraceStart = Center + Offset + FVector(0.0f, 0.0f, 5000.0f);
	const FVector TraceEnd = Center + Offset - FVector(0.0f, 0.0f, 5000.0f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.Location;
	}

	return Center + Offset;
}

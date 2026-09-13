#include "Weapon/CPMeteorPassiveSkillModule.h"
#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPMeteor.h"
#include "Weapon/CPMeteorGroundZone.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"

int32 UCPMeteorPassiveSkillModule::GetMaxLevel() const
{
	return FMath::Max(LevelData.Num(), 1);
}

const FCPMeteorLevelData& UCPMeteorPassiveSkillModule::GetLevelData(int32 Level) const
{
	static const FCPMeteorLevelData DefaultLevelData;

	if (LevelData.Num() == 0)
	{
		return DefaultLevelData;
	}

	return LevelData[FMath::Clamp(Level - 1, 0, LevelData.Num() - 1)];
}

void UCPMeteorPassiveSkillModule::Activate(const FCPPassiveSkillActivationContext& Context)
{
	UWorld* World = Context.Weapon ? Context.Weapon->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPMeteorPassiveSkillModule::Activate - no World (Context.Weapon is %s)"), Context.Weapon ? TEXT("valid") : TEXT("null"));
		return;
	}

	const FCPMeteorLevelData LevelInfo = GetLevelData(Context.Weapon->GetWeaponLevel());
	const int32 MeteorCount = FMath::Max(LevelInfo.MeteorCount, 1);

	TWeakObjectPtr<ACPWeaponBase> WeakWeapon(Context.Weapon);
	TWeakObjectPtr<ACharacter> WeakOwnerCharacter(Context.OwnerCharacter);
	TWeakObjectPtr<AActor> WeakDamageCauser(Context.DamageCauser);
	TWeakObjectPtr<AController> WeakInstigatorController(Context.InstigatorController);
	const FVector Origin = Context.Origin;

	for (int32 Index = 0; Index < MeteorCount; ++Index)
	{
		const float Delay = LevelInfo.MeteorSpawnInterval * Index;

		if (Delay <= 0.0f)
		{
			SpawnMeteor(WeakWeapon, WeakOwnerCharacter, WeakDamageCauser, WeakInstigatorController, Origin, LevelInfo);
		}
		else
		{
			FTimerDelegate SpawnDelegate = FTimerDelegate::CreateUObject(this, &UCPMeteorPassiveSkillModule::SpawnMeteor, WeakWeapon, WeakOwnerCharacter, WeakDamageCauser, WeakInstigatorController, Origin, LevelInfo);

			FTimerHandle UnusedHandle;
			World->GetTimerManager().SetTimer(UnusedHandle, SpawnDelegate, Delay, false);
		}
	}
}

void UCPMeteorPassiveSkillModule::SpawnMeteor(TWeakObjectPtr<ACPWeaponBase> WeakWeapon, TWeakObjectPtr<ACharacter> WeakOwnerCharacter, TWeakObjectPtr<AActor> WeakDamageCauser, TWeakObjectPtr<AController> WeakInstigatorController, FVector Origin, FCPMeteorLevelData LevelInfo) const
{
	ACPWeaponBase* Weapon = WeakWeapon.Get();
	UWorld* World = Weapon ? Weapon->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	TSubclassOf<ACPMeteor> ClassToSpawn = MeteorClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACPMeteor::StaticClass();
	}

	const FVector LandingLocation = ComputeLandingLocation(Origin, World, LevelInfo.DropRadius);
	const FVector SpawnLocation = LandingLocation + FVector(0.0f, 0.0f, LevelInfo.FallHeight);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = WeakDamageCauser.Get();
	SpawnParams.Instigator = WeakOwnerCharacter.Get();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPMeteor* Meteor = World->SpawnActor<ACPMeteor>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!Meteor)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPMeteorPassiveSkillModule::SpawnMeteor - SpawnActor failed for class '%s' at %s"), *GetNameSafe(ClassToSpawn.Get()), *SpawnLocation.ToString());
		return;
	}

	Meteor->SetActorScale3D(FVector(FMath::Max(LevelInfo.MeteorScale, 0.01f)));

	UE_LOG(LogTemp, Log, TEXT("UCPMeteorPassiveSkillModule::SpawnMeteor - spawned '%s', landing at %s (fall height %.0f, radius %.0f)"), *GetNameSafe(Meteor), *LandingLocation.ToString(), LevelInfo.FallHeight, LevelInfo.ImpactRadius);

	Meteor->InitializeMeteor(LandingLocation, LevelInfo.FallSpeed, LevelInfo.ImpactRadius, LevelInfo.ImpactDamage, LevelInfo.ImpactKnockbackDistance, GroundZoneClass, LevelInfo.GroundZoneDuration, LevelInfo.GroundZoneDamagePerTick, LevelInfo.GroundZoneTickInterval, LevelInfo.GroundZoneRadius, LevelInfo.EffectScale, WeakInstigatorController.Get(), WeakDamageCauser.Get());
}

FVector UCPMeteorPassiveSkillModule::ComputeLandingLocation(const FVector& Center, UWorld* World, float DropRadius) const
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

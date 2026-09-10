#include "Weapon/CPOrbitPassiveSkillModule.h"
#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPOrbitingCrescent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void UCPOrbitPassiveSkillModule::Activate(const FCPPassiveSkillActivationContext& Context)
{
	UWorld* World = Context.Weapon ? Context.Weapon->GetWorld() : nullptr;
	if (!World || !Context.OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPOrbitPassiveSkillModule::Activate - missing World (%s) or OwnerCharacter (%s)"), World ? TEXT("ok") : TEXT("null"), Context.OwnerCharacter ? TEXT("ok") : TEXT("null"));
		return;
	}

	TSubclassOf<ACPOrbitingCrescent> ClassToSpawn = CrescentClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACPOrbitingCrescent::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Context.DamageCauser;
	SpawnParams.Instigator = Context.OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPOrbitingCrescent* Crescent = World->SpawnActor<ACPOrbitingCrescent>(ClassToSpawn, Context.Origin, FRotator::ZeroRotator, SpawnParams);
	if (!Crescent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPOrbitPassiveSkillModule::Activate - SpawnActor failed for class '%s' at %s"), *GetNameSafe(ClassToSpawn.Get()), *Context.Origin.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UCPOrbitPassiveSkillModule::Activate - spawned '%s' orbiting '%s' (radius %.0f, hit radius %.0f)"), *GetNameSafe(Crescent), *GetNameSafe(Context.OwnerCharacter), OrbitRadius, HitRadius);

	Crescent->InitializeCrescent(Context.OwnerCharacter, OrbitRadius, OrbitSpeedDegPerSec, SelfSpinSpeedDegPerSec, HitRadius, VerticalOffset, Duration, Damage, KnockbackDistance, DamageTickInterval, Context.InstigatorController, Context.DamageCauser);
}

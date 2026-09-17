#include "Weapon/CPOrbitPassiveSkillModule.h"
#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPOrbitingCrescent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"

int32 UCPOrbitPassiveSkillModule::GetMaxLevel() const
{
	return FMath::Max(LevelData.Num(), 1);
}

const FCPOrbitLevelData& UCPOrbitPassiveSkillModule::GetLevelData(int32 Level) const
{
	static const FCPOrbitLevelData DefaultLevelData;

	if (LevelData.Num() == 0)
	{
		return DefaultLevelData;
	}

	return LevelData[FMath::Clamp(Level - 1, 0, LevelData.Num() - 1)];
}

void UCPOrbitPassiveSkillModule::Activate(const FCPPassiveSkillActivationContext& Context)
{
	UWorld* World = Context.Weapon ? Context.Weapon->GetWorld() : nullptr;
	if (!World || !Context.OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCPOrbitPassiveSkillModule::Activate - missing World (%s) or OwnerCharacter (%s)"), World ? TEXT("ok") : TEXT("null"), Context.OwnerCharacter ? TEXT("ok") : TEXT("null"));
		return;
	}

	CachedWeapon = Context.Weapon;

	const FCPOrbitLevelData& LevelInfo = GetLevelData(Context.Weapon->GetWeaponLevel());
	const int32 DesiredSatelliteCount = FMath::Max(LevelInfo.SatelliteCount, 1);
	const float MaxDuration = FMath::Max(LevelInfo.MaxDuration, 0.01f);
	ActiveMaxDuration = MaxDuration;

	ActiveCrescents.RemoveAll([](const TObjectPtr<ACPOrbitingCrescent>& Crescent) { return !IsValid(Crescent); });

	const float PreviousRemaining = ActiveCrescents.Num() > 0 ? FMath::Max(World->GetTimerManager().GetTimerRemaining(GroupExpireTimerHandle), 0.0f) : 0.0f;
	const float NewDuration = FMath::Clamp(PreviousRemaining + LevelInfo.ChargeDurationPerActivation, 0.01f, MaxDuration);

	if (ActiveCrescents.Num() != DesiredSatelliteCount)
	{
		DespawnCrescentGroup();
		SpawnCrescentGroup(Context, LevelInfo);
	}

	World->GetTimerManager().SetTimer(GroupExpireTimerHandle, this, &UCPOrbitPassiveSkillModule::DespawnCrescentGroup, NewDuration, false);
}

void UCPOrbitPassiveSkillModule::SpawnCrescentGroup(const FCPPassiveSkillActivationContext& Context, const FCPOrbitLevelData& LevelInfo)
{
	UWorld* World = Context.Weapon->GetWorld();

	TSubclassOf<ACPOrbitingCrescent> ClassToSpawn = CrescentClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACPOrbitingCrescent::StaticClass();
	}

	const int32 Count = FMath::Max(LevelInfo.SatelliteCount, 1);
	const float AngleStep = 360.0f / Count;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Context.DamageCauser;
	SpawnParams.Instigator = Context.OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		ACPOrbitingCrescent* Crescent = World->SpawnActor<ACPOrbitingCrescent>(ClassToSpawn, Context.Origin, FRotator::ZeroRotator, SpawnParams);
		if (!Crescent)
		{
			UE_LOG(LogTemp, Warning, TEXT("UCPOrbitPassiveSkillModule::SpawnCrescentGroup - SpawnActor failed for class '%s' at %s"), *GetNameSafe(ClassToSpawn.Get()), *Context.Origin.ToString());
			continue;
		}

		Crescent->InitializeCrescent(Context.OwnerCharacter, LevelInfo.OrbitRadius, LevelInfo.OrbitSpeedDegPerSec, LevelInfo.SelfSpinSpeedDegPerSec, LevelInfo.HitRadius, LevelInfo.VerticalOffset, AngleStep * Index, LevelInfo.Damage, LevelInfo.KnockbackDistance, LevelInfo.DamageTickInterval, LevelInfo.EffectScale, Context.InstigatorController, Context.DamageCauser);

		ActiveCrescents.Add(Crescent);
	}

	UE_LOG(LogTemp, Log, TEXT("UCPOrbitPassiveSkillModule::SpawnCrescentGroup - spawned %d crescent(s) orbiting '%s' (radius %.0f, speed %.0f deg/s)"), ActiveCrescents.Num(), *GetNameSafe(Context.OwnerCharacter), LevelInfo.OrbitRadius, LevelInfo.OrbitSpeedDegPerSec);
}

void UCPOrbitPassiveSkillModule::DespawnCrescentGroup()
{
	for (ACPOrbitingCrescent* Crescent : ActiveCrescents)
	{
		if (IsValid(Crescent))
		{
			Crescent->Destroy();
		}
	}

	ActiveCrescents.Reset();
}

float UCPOrbitPassiveSkillModule::GetActiveDurationRemaining() const
{
	ACPWeaponBase* Weapon = CachedWeapon.Get();
	if (!Weapon || ActiveCrescents.IsEmpty())
	{
		return 0.0f;
	}

	return FMath::Max(Weapon->GetWorldTimerManager().GetTimerRemaining(GroupExpireTimerHandle), 0.0f);
}

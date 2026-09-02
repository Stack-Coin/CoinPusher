// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPDelayedExplosionModule.h"
#include "Weapon/CPDelayedExplosion.h"
#include "Weapon/CPWeaponBase.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void UCPDelayedExplosionModule::Execute(const FCPAttackExecutionContext& Context)
{
	if (!Context.Weapon)
	{
		return;
	}

	UWorld* World = Context.Weapon->GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ACPDelayedExplosion> ClassToSpawn = ExplosionClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACPDelayedExplosion::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Context.DamageCauser;
	SpawnParams.Instigator = Context.OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPDelayedExplosion* Explosion = World->SpawnActor<ACPDelayedExplosion>(ClassToSpawn, Context.Origin, FRotator::ZeroRotator, SpawnParams);
	if (!Explosion)
	{
		return;
	}

	Explosion->InitializeExplosion(ExplosionDelay, ExplosionRadius, KnockbackDistance, Context.AttackPower, Context.InstigatorController, Context.DamageCauser);
}

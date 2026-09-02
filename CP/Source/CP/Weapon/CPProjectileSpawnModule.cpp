// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPProjectileSpawnModule.h"
#include "Weapon/CPProjectile.h"
#include "Weapon/CPWeaponBase.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void UCPProjectileSpawnModule::Execute(const FCPAttackExecutionContext& Context)
{
	if (!ProjectileClass || !Context.Weapon)
	{
		return;
	}

	UWorld* World = Context.Weapon->GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Context.DamageCauser;
	SpawnParams.Instigator = Context.OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPProjectile* Projectile = World->SpawnActor<ACPProjectile>(ProjectileClass, Context.Origin, Context.Direction.Rotation(), SpawnParams);
	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeProjectile(Context.AttackPower, Context.InstigatorController, Context.DamageCauser);
}

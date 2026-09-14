// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Ranged/CPMonsterRanged.h"
#include "Monster/Ranged/CPMonsterProjectile.h"

ACPMonsterRanged::ACPMonsterRanged()
{
}

void ACPMonsterRanged::AttackHitCheck()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float Duration = GetAIAttackInterval();

	if (Duration > 0.f && Now - LastFireTime < Duration)
	{
		return;
	}
	LastFireTime = Now;

	Fire();
}

void ACPMonsterRanged::Fire()
{
	if (!ProjectileClass)
	{
		return;
	}

	FVector SpawnLocation = GetActorLocation();
	const FRotator SpawnRotation = GetActorRotation();

	if (GetMesh() && GetMesh()->DoesSocketExist(MuzzleSocketName))
	{
		SpawnLocation = GetMesh()->GetSocketLocation(MuzzleSocketName);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPMonsterProjectile* Projectile = GetWorld()->SpawnActor<ACPMonsterProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Projectile)
	{
		Projectile->Init(GetAIAttackPower(), GetController(), this);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/Ranged/CPMonsterRanged.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Monster/Ranged/CPMonsterProjectile.h"

void ACPMonsterRanged::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void ACPMonsterRanged::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = 0.f;
	CurrentRotation.Roll = 0.f;

	SetActorRotation(CurrentRotation);
}

void ACPMonsterRanged::AttackHitCheck()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float Duration = GetAIAttackSpeed();
	
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

	if (ACPMonsterProjectile* Projectile = GetWorld()->SpawnActor<ACPMonsterProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		Projectile->Init(GetAIAttackPower(), GetAIKnockbackDistance(), GetController(), this);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPRangedWeapon.h"
#include "Weapon/CPProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

ACPRangedWeapon::ACPRangedWeapon()
{
	WeaponType = ECPWeaponType::Ranged;
}

void ACPRangedWeapon::ExecuteAttack(int32 ComboIndex)
{
	if (!ProjectileClass)
	{
		return;
	}

	const FVector MuzzleLocation = GetMuzzleLocation();
	const FVector BaseDirection = GetBaseFireDirection();

	for (const FVector& Direction : ComputeFireDirections(BaseDirection))
	{
		SpawnProjectile(MuzzleLocation, Direction);
	}

	PlayAttackEffect(MuzzleLocation);
}

FVector ACPRangedWeapon::GetMuzzleLocation() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketLocation(MuzzleSocketName) + ProjectileSpawnOffset;
	}

	return GetActorLocation() + ProjectileSpawnOffset;
}

FVector ACPRangedWeapon::GetBaseFireDirection() const
{
	if (ACharacter* OwnerCharacter = GetOwningCharacter())
	{
		return OwnerCharacter->GetActorForwardVector();
	}

	return GetActorForwardVector();
}

TArray<FVector> ACPRangedWeapon::ComputeFireDirections(const FVector& BaseDirection) const
{
	TArray<FVector> Directions;
	const int32 Count = FMath::Max(ProjectileCount, 1);

	switch (FirePattern)
	{
	case ECPProjectileFirePattern::Straight:
	{
		if (Count == 1)
		{
			Directions.Add(BaseDirection);
		}
		else
		{
			const float HalfSpread = SpreadAngle * 0.5f;
			const float Step = SpreadAngle / static_cast<float>(Count - 1);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const float Angle = -HalfSpread + Step * Index;
				Directions.Add(BaseDirection.RotateAngleAxis(Angle, FVector::UpVector));
			}
		}
		break;
	}

	case ECPProjectileFirePattern::Radial:
	{
		const float Step = 360.0f / static_cast<float>(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Directions.Add(BaseDirection.RotateAngleAxis(Step * Index, FVector::UpVector));
		}
		break;
	}
	}

	return Directions;
}

void ACPRangedWeapon::SpawnProjectile(const FVector& Location, const FVector& Direction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ACharacter* OwnerCharacter = GetOwningCharacter();
	AActor* DamageCauser = OwnerCharacter ? static_cast<AActor*>(OwnerCharacter) : static_cast<AActor*>(this);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = DamageCauser;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPProjectile* Projectile = World->SpawnActor<ACPProjectile>(ProjectileClass, Location, Direction.Rotation(), SpawnParams);
	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeProjectile(GetFinalAttackPower(), OwnerCharacter ? OwnerCharacter->GetController() : nullptr, DamageCauser);
}

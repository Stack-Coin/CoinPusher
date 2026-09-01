// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPMeleeWeapon.h"
#include "Weapon/CPKnockbackInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACPMeleeWeapon::ACPMeleeWeapon()
{
	WeaponType = ECPWeaponType::Melee;
}

void ACPMeleeWeapon::ExecuteAttack(int32 ComboIndex)
{
	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);

	if (AttackTiming > 0.0f)
	{
		GetWorldTimerManager().SetTimer(MeleeHitTimerHandle, this, &ACPMeleeWeapon::ExecuteMeleeHit, AttackTiming, false);
	}
	else
	{
		ExecuteMeleeHit();
	}
}

FVector ACPMeleeWeapon::GetAttackDirection() const
{
	if (ACharacter* OwnerCharacter = GetOwningCharacter())
	{
		return OwnerCharacter->GetActorForwardVector();
	}

	return GetActorForwardVector();
}

FVector ACPMeleeWeapon::GetAttackOrigin() const
{
	ACharacter* OwnerCharacter = GetOwningCharacter();
	const FVector BaseLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : GetActorLocation();

	return BaseLocation + GetAttackDirection() * AttackRangeOffset;
}

void ACPMeleeWeapon::ExecuteMeleeHit()
{
	ACharacter* OwnerCharacter = GetOwningCharacter();
	AActor* DamageCauser = OwnerCharacter ? static_cast<AActor*>(OwnerCharacter) : static_cast<AActor*>(this);

	const FVector Direction = GetAttackDirection();
	const FVector Origin = GetAttackOrigin();
	const FRotator Rotation = Direction.Rotation();

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	if (OwnerCharacter)
	{
		ActorsToIgnore.Add(OwnerCharacter);
	}

	const EDrawDebugTrace::Type DebugType = bDrawDebugAttackShape ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	TArray<FHitResult> HitResults;
	switch (AttackRangeShape)
	{
	case ECPMeleeAttackShape::Sphere:
		UKismetSystemLibrary::SphereTraceMulti(
			this, Origin, Origin, AttackRangeSize.X,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;

	case ECPMeleeAttackShape::Box:
		UKismetSystemLibrary::BoxTraceMulti(
			this, Origin, Origin, AttackRangeSize * 0.5f, Rotation,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;

	case ECPMeleeAttackShape::Capsule:
		UKismetSystemLibrary::CapsuleTraceMulti(
			this, Origin, Origin, AttackRangeSize.X, AttackRangeSize.Y,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;
	}

	const float Damage = GetFinalAttackPower();
	AController* InstigatorController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;

	// A single actor can report multiple hit results (e.g. capsule + mesh), so only process each unique target once per swing
	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.Add(HitActor);

		UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController, DamageCauser, nullptr);

		if (ICPKnockbackable* Knockbackable = Cast<ICPKnockbackable>(HitActor))
		{
			Knockbackable->ApplyKnockback(Direction, KnockbackDistance, DamageCauser);
		}
	}

	PlayAttackEffect(Origin);
}

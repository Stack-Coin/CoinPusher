// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPMeleeWeapon.h"
#include "Weapon/CPKnockbackInterface.h"
#include "Weapon/CPAttackModule.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

ACPMeleeWeapon::ACPMeleeWeapon()
{
	WeaponType = ECPWeaponType::Melee;
}

void ACPMeleeWeapon::ExecuteAttack(int32 ComboIndex)
{
	PendingHitComboIndex = ComboIndex;

	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);

	const float AttackTiming = GetComboStepData(ComboIndex).AttackTiming;
	if (AttackTiming > 0.0f)
	{
		GetWorldTimerManager().SetTimer(MeleeHitTimerHandle, this, &ACPMeleeWeapon::ExecuteMeleeHit, AttackTiming, false);
	}
	else
	{
		ExecuteMeleeHit();
	}
}

void ACPMeleeWeapon::CancelAttack()
{
	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);

	Super::CancelAttack();
}

FVector ACPMeleeWeapon::GetAttackDirection() const
{
	// Captured once in ACPWeaponBase::StartAttack (e.g. mouse-cursor direction at click time) - a moving
	// cursor during AttackTiming/ComboAttackInterval can't change where an already-started swing lands
	return CapturedAttackDirection;
}

FVector ACPMeleeWeapon::GetAttackOrigin(float InRangeOffset) const
{
	ACharacter* OwnerCharacter = GetOwningCharacter();
	const FVector BaseLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : GetActorLocation();

	return BaseLocation + GetAttackDirection() * InRangeOffset;
}

const FCPMeleeComboStepData& ACPMeleeWeapon::GetComboStepData(int32 ComboIndex) const
{
	static const FCPMeleeComboStepData DefaultStepData;

	if (ComboSteps.Num() == 0)
	{
		return DefaultStepData;
	}

	return ComboSteps[FMath::Clamp(ComboIndex, 0, ComboSteps.Num() - 1)];
}

bool ACPMeleeWeapon::IsWithinArc(const FVector& Origin, const FVector& Direction, const FVector& HitLocation, float FullArcAngleDegrees)
{
	const FVector ToHit = (HitLocation - Origin).GetSafeNormal2D();
	const FVector FlatDirection = Direction.GetSafeNormal2D();

	if (ToHit.IsNearlyZero() || FlatDirection.IsNearlyZero())
	{
		return true;
	}

	const float DotProduct = FVector::DotProduct(FlatDirection, ToHit);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f)));

	return AngleDegrees <= FullArcAngleDegrees * 0.5f;
}

void ACPMeleeWeapon::ExecuteMeleeHit()
{
	const FCPMeleeComboStepData& StepData = GetComboStepData(PendingHitComboIndex);

	ACharacter* OwnerCharacter = GetOwningCharacter();
	AActor* DamageCauser = OwnerCharacter ? static_cast<AActor*>(OwnerCharacter) : static_cast<AActor*>(this);

	const FVector Direction = GetAttackDirection();
	const FVector Origin = GetAttackOrigin(StepData.RangeOffset);
	const FRotator Rotation = Direction.Rotation();

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	if (OwnerCharacter)
	{
		ActorsToIgnore.Add(OwnerCharacter);
	}

	const EDrawDebugTrace::Type DebugType = bDrawDebugAttackShape ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	TArray<FHitResult> HitResults;
	switch (StepData.Shape)
	{
	case ECPMeleeAttackShape::Sphere:
	case ECPMeleeAttackShape::Arc:
		UKismetSystemLibrary::SphereTraceMulti(
			this, Origin, Origin, StepData.ShapeSize.X,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;

	case ECPMeleeAttackShape::Box:
		UKismetSystemLibrary::BoxTraceMulti(
			this, Origin, Origin, StepData.ShapeSize * 0.5f, Rotation,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;

	case ECPMeleeAttackShape::Capsule:
		UKismetSystemLibrary::CapsuleTraceMulti(
			this, Origin, Origin, StepData.ShapeSize.X, StepData.ShapeSize.Y,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;
	}

	// SphereTraceMulti's own debug draw shows the full candidate sphere, not the angle-filtered wedge actually
	// used for hit processing below - draw the wedge's boundary/arc on top of it so Arc's real hit area is visible
	if (bDrawDebugAttackShape && StepData.Shape == ECPMeleeAttackShape::Arc)
	{
		const float HalfArcAngle = StepData.ArcAngle * 0.5f;
		const float ArcRadius = StepData.ShapeSize.X;
		const FVector FlatDirection = Direction.GetSafeNormal2D();

		constexpr int32 ArcSegments = 16;
		FVector PreviousPoint = Origin + FlatDirection.RotateAngleAxis(-HalfArcAngle, FVector::UpVector) * ArcRadius;
		DrawDebugLine(GetWorld(), Origin, PreviousPoint, FColor::Red, false, 5.0f, 0, 3.0f);

		for (int32 Index = 1; Index <= ArcSegments; ++Index)
		{
			const float Angle = -HalfArcAngle + (StepData.ArcAngle * Index / ArcSegments);
			const FVector NextPoint = Origin + FlatDirection.RotateAngleAxis(Angle, FVector::UpVector) * ArcRadius;
			DrawDebugLine(GetWorld(), PreviousPoint, NextPoint, FColor::Red, false, 5.0f, 0, 3.0f);
			PreviousPoint = NextPoint;
		}

		DrawDebugLine(GetWorld(), Origin, PreviousPoint, FColor::Red, false, 5.0f, 0, 3.0f);
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

		if (StepData.Shape == ECPMeleeAttackShape::Arc && !IsWithinArc(Origin, Direction, HitActor->GetActorLocation(), StepData.ArcAngle))
		{
			continue;
		}

		HitActors.Add(HitActor);

		UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController, DamageCauser, nullptr);

		if (ICPKnockbackable* Knockbackable = Cast<ICPKnockbackable>(HitActor))
		{
			Knockbackable->ApplyKnockback(Direction, StepData.KnockbackDistance, DamageCauser);
		}
	}

	PlayAttackEffect(Origin);

	if (!StepData.PostHitModules.IsEmpty())
	{
		// PostHitModuleOffset lets a step's follow-up effect (e.g. an explosion) be placed independently of
		// the hit-scan shape's own Origin - X forward along Direction, Y to its right, Z world up
		const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
		const FVector ModuleOrigin = Origin
			+ Direction * StepData.PostHitModuleOffset.X
			+ RightDirection * StepData.PostHitModuleOffset.Y
			+ FVector::UpVector * StepData.PostHitModuleOffset.Z;

		FCPAttackExecutionContext Context;
		Context.Weapon = this;
		Context.OwnerCharacter = OwnerCharacter;
		Context.InstigatorController = InstigatorController;
		Context.DamageCauser = DamageCauser;
		Context.Origin = ModuleOrigin;
		Context.Direction = Direction;
		Context.AttackPower = Damage;
		Context.ComboIndex = PendingHitComboIndex;

		for (UCPAttackModule* Module : StepData.PostHitModules)
		{
			if (Module)
			{
				Module->Execute(Context);
			}
		}
	}
}

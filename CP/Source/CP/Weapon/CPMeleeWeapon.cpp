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
#include "Debug/CPDebugCollisionSubsystem.h"

ACPMeleeWeapon::ACPMeleeWeapon()
{
	WeaponType = ECPWeaponType::Melee;
}

void ACPMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &ACPMeleeWeapon::HandleDebugCollisionVisibilityChanged);
		bDrawDebugAttackShape = Subsystem->IsCategoryVisible(ECPDebugCollisionCategory::PlayerWeapon);
	}
}

void ACPMeleeWeapon::HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (Category == ECPDebugCollisionCategory::PlayerWeapon)
	{
		bDrawDebugAttackShape = bVisible;
	}
}

void ACPMeleeWeapon::ExecuteAttack(int32 ComboIndex)
{
	if (GetWorldTimerManager().IsTimerActive(MeleeHitTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);
		ExecuteMeleeHit();
	}

	PendingHitComboIndex = ComboIndex;

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

	return BaseLocation + GetAttackDirection() * (InRangeOffset * GetFinalAttackRangeMultiplier());
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
	const FVector ScaledShapeSize = StepData.ShapeSize * GetFinalAttackRangeMultiplier();

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
			this, Origin, Origin, ScaledShapeSize.X,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;

	case ECPMeleeAttackShape::Box:
		UKismetSystemLibrary::BoxTraceMulti(
			this, Origin, Origin, ScaledShapeSize * 0.5f, Rotation,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;

	case ECPMeleeAttackShape::Capsule:
		UKismetSystemLibrary::CapsuleTraceMulti(
			this, Origin, Origin, ScaledShapeSize.X, ScaledShapeSize.Y,
			UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore,
			DebugType, HitResults, true);
		break;
	}

	// SphereTraceMulti's own debug draw shows the full candidate sphere, not the angle-filtered wedge actually
	// used for hit processing below - draw the wedge's boundary/arc on top of it so Arc's real hit area is visible
	if (bDrawDebugAttackShape && StepData.Shape == ECPMeleeAttackShape::Arc)
	{
		const float HalfArcAngle = StepData.ArcAngle * 0.5f;
		const float ArcRadius = ScaledShapeSize.X;
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

	// Direction/Origin above are locked to CapturedAttackDirection (resolved back in StartAttack) so a moving
	// cursor during AttackTiming/ComboAttackInterval can't change where this already-started swing's hit
	// lands - only the hit-scan judgment itself (already run, above) needs that fairness lock. Everything
	// that fires after the hit is resolved - the attack effect, and any PostHitModules follow-up (e.g. a
	// delayed explosion) - resolves a fresh direction right here instead, at the moment the swing's hit
	// judgment actually happens, so it renders/lands wherever the player was last aiming (mouse cursor/
	// gamepad stick, or movement direction if neither - see ACPPlayerCharacter::GetAttackDirection) instead
	// of the swing's locked hit direction
	const FVector EffectDirection = ResolveAimDirection();
	const FVector EffectBaseLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : GetActorLocation();
	const FVector EffectOrigin = EffectBaseLocation + EffectDirection * (StepData.RangeOffset * GetFinalAttackRangeMultiplier());
	TriggerAttackEffect(EffectOrigin, EffectDirection.Rotation());

	if (!StepData.PostHitModules.IsEmpty())
	{
		// PostHitModuleOffset lets a step's follow-up effect (e.g. an explosion) be placed independently of
		// the hit-scan shape's own Origin - X forward along EffectDirection, Y to its right, Z world up.
		// Based on EffectOrigin/EffectDirection (the swing's final aim direction, same as the attack effect
		// above) rather than the swing's locked hit Direction/Origin, so e.g. a delayed explosion lands where
		// the player was actually aiming when the hit landed, not where they were aiming when the attack
		// button was first pressed
		const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, EffectDirection).GetSafeNormal();
		const FVector ModuleOrigin = EffectOrigin
			+ EffectDirection * StepData.PostHitModuleOffset.X
			+ RightDirection * StepData.PostHitModuleOffset.Y
			+ FVector::UpVector * StepData.PostHitModuleOffset.Z;

		FCPAttackExecutionContext Context;
		Context.Weapon = this;
		Context.OwnerCharacter = OwnerCharacter;
		Context.InstigatorController = InstigatorController;
		Context.DamageCauser = DamageCauser;
		Context.Origin = ModuleOrigin;
		Context.Direction = EffectDirection;
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

// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/CPDebugCollisionShapeComponent.h"
#include "Debug/CPDebugCollisionSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

namespace
{
	// Refresh rate for the redraw timer - a moving shape redrawn this often reads as continuous
	constexpr float DebugShapeDrawInterval = 0.1f;
}

UCPDebugCollisionShapeComponent::UCPDebugCollisionShapeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCPDebugCollisionShapeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &UCPDebugCollisionShapeComponent::HandleCollisionVisibilityChanged);
		SetDrawEnabled(Subsystem->IsCategoryVisible(Category));
	}
}

void UCPDebugCollisionShapeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DrawTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UCPDebugCollisionShapeComponent::HandleCollisionVisibilityChanged(ECPDebugCollisionCategory ChangedCategory, bool bVisible)
{
	if (ChangedCategory == Category)
	{
		SetDrawEnabled(bVisible);
	}
}

void UCPDebugCollisionShapeComponent::SetDrawEnabled(bool bEnabled)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DrawTimerHandle);

	if (bEnabled)
	{
		DrawShape();
		World->GetTimerManager().SetTimer(DrawTimerHandle, this, &UCPDebugCollisionShapeComponent::DrawShape, DebugShapeDrawInterval, true);
	}
}

void UCPDebugCollisionShapeComponent::DrawShape() const
{
	if (!TargetComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	const FVector Location = TargetComponent->GetComponentLocation();

	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(TargetComponent))
	{
		DrawDebugCapsule(World, Location, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), TargetComponent->GetComponentQuat(), ShapeColor, false, DebugShapeDrawInterval * 1.5f, 0, 1.0f);
	}
	else if (const USphereComponent* Sphere = Cast<USphereComponent>(TargetComponent))
	{
		DrawDebugSphere(World, Location, Sphere->GetScaledSphereRadius(), 16, ShapeColor, false, DebugShapeDrawInterval * 1.5f, 0, 1.0f);
	}
}

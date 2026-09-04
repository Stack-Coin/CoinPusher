// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPPartyCamera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ACPPartyCamera::ACPPartyCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Same fixed quarter-view angle as ACPPlayerCharacter's own CameraBoom, so swapping to/from the
	// party camera doesn't change the viewing angle
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = MinArmLength;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	// No built-in camera lag - Tick already hand-smooths both position (FollowInterpSpeed) and
	// zoom (ZoomInterpSpeed), stacking the spring arm's own lag on top would double-smooth
	CameraBoom->bEnableCameraLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ACPPartyCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AActor*> ValidActors;
	ValidActors.Reserve(TrackedActors.Num());
	for (const TWeakObjectPtr<AActor>& Weak : TrackedActors)
	{
		if (AActor* Actor = Weak.Get())
		{
			ValidActors.Add(Actor);
		}
	}

	if (ValidActors.IsEmpty())
	{
		return;
	}

	FVector Midpoint = FVector::ZeroVector;
	for (const AActor* Actor : ValidActors)
	{
		Midpoint += Actor->GetActorLocation();
	}
	Midpoint /= ValidActors.Num();

	float MaxSpread = 0.0f;
	for (int32 i = 0; i < ValidActors.Num(); ++i)
	{
		for (int32 j = i + 1; j < ValidActors.Num(); ++j)
		{
			MaxSpread = FMath::Max(MaxSpread, FVector::Dist(ValidActors[i]->GetActorLocation(), ValidActors[j]->GetActorLocation()));
		}
	}

	const float ZoomAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinTrackedDistance, MaxTrackedDistance), FVector2D(0.0f, 1.0f), MaxSpread);
	const float TargetArmLength = FMath::Lerp(MinArmLength, MaxArmLength, ZoomAlpha);
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);

	const FVector TargetLocation(Midpoint.X, Midpoint.Y, GetActorLocation().Z);
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, FollowInterpSpeed));
}

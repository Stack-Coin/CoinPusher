// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPTestBot.h"
#include "Player/CPPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogCPTestBot);

ACPTestBot::ACPTestBot()
{
	PrimaryActorTick.bCanEverTick = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// The test bot stands in place and never rotates on its own.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ACPTestBot::BeginPlay()
{
	Super::BeginPlay();

	if (AttackInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ACPTestBot::PerformBotAttack, AttackInterval, true);
	}
}

void ACPTestBot::PerformBotAttack()
{
	const FVector AttackDirection = GetActorForwardVector();
	const FVector BoxCenter = GetActorLocation() + AttackDirection * AttackOffset;
	const FRotator BoxRotation = AttackDirection.Rotation();
	const FVector BoxHalfExtent(AttackLength * 0.5f, AttackWidth * 0.5f, AttackHeight * 0.5f);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::BoxTraceMulti(
		this,
		BoxCenter,
		BoxCenter,
		BoxHalfExtent,
		BoxRotation,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		bDrawDebugAttackBox ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		0.2f);

	// A single actor can report multiple hit results (e.g. capsule + mesh), so only
	// log each unique target once per attack.
	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && !HitActors.Contains(HitActor))
		{
			HitActors.Add(HitActor);

			if (ACPPlayerCharacter* PlayerCharacter = Cast<ACPPlayerCharacter>(HitActor))
			{
				UE_LOG(LogCPTestBot, Log, TEXT("[CPTestBot] %s's attack hit player %s"), *GetName(), *PlayerCharacter->GetName());
			}
		}
	}
}

float ACPTestBot::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogCPTestBot, Log, TEXT("[CPTestBot] %s was hit by %s for %.1f damage"), *GetName(), *GetNameSafe(DamageCauser), DamageAmount);

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

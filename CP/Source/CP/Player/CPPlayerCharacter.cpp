// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogCPPlayerCharacter);

ACPPlayerCharacter::ACPPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// The character does not rotate with the controller. It faces its movement direction instead.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// Camera boom holding a fixed quarter view angle. It ignores the character's rotation entirely.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 900.0f;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 5.0f;
	CameraBoom->CameraLagMaxDistance = 200.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ACPPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyStatsToGameplay();
}

void ACPPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACPPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::Attack);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::StartDash);
	}
	else
	{
		UE_LOG(LogCPPlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This character requires the Enhanced Input system."), *GetNameSafe(this));
	}
}

void ACPPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (!MovementVector.IsNearlyZero())
	{
		LastMoveInputVector = MovementVector;
	}

	DoMove(MovementVector.X, MovementVector.Y);
}

void ACPPlayerCharacter::Attack(const FInputActionValue& Value)
{
	DoAttack();
}

void ACPPlayerCharacter::StartDash(const FInputActionValue& Value)
{
	DoDash();
}

void ACPPlayerCharacter::DoMove(float Right, float Forward)
{
	if (Controller == nullptr)
	{
		return;
	}

	const FVector2D InputVector(Right, Forward);
	if (InputVector.IsNearlyZero())
	{
		return;
	}

	AddMovementInput(GetWorldDirectionFromInput(InputVector), 1.0f);
}

void ACPPlayerCharacter::DoAttack()
{
	const float EffectiveAttackCooldown = Stats.AttackSpeed > 0.0f ? (AttackCooldown / Stats.AttackSpeed) : AttackCooldown;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < EffectiveAttackCooldown)
	{
		return;
	}
	LastAttackTime = CurrentTime;

	PerformAttack();
}

void ACPPlayerCharacter::DoDash()
{
	if (bIsDashing)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDashTime < DashCooldown)
	{
		return;
	}
	LastDashTime = CurrentTime;

	DashDirection = GetWorldDirectionFromInput(LastMoveInputVector);
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector();
	}

	bIsDashing = true;
	bIsInvincible = true;

	const float DashSpeed = DashDuration > 0.0f ? (DashDistance / DashDuration) : DashDistance;
	LaunchCharacter(DashDirection * DashSpeed, true, true);

	GetWorldTimerManager().SetTimer(DashDurationTimerHandle, this, &ACPPlayerCharacter::EndDash, DashDuration, false);
}

FVector ACPPlayerCharacter::GetWorldDirectionFromInput(const FVector2D& InputVector) const
{
	return FVector(
		InputVector.Y, // W/S → World X
		InputVector.X, // A/D → World Y
		0.0f
	).GetSafeNormal();
}

FVector ACPPlayerCharacter::GetAttackDirection() const
{
	FVector Direction = GetActorForwardVector();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FHitResult CursorHit;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit))
		{
			FVector ToCursor = CursorHit.Location - GetActorLocation();
			ToCursor.Z = 0.0f;

			if (!ToCursor.IsNearlyZero())
			{
				Direction = ToCursor.GetSafeNormal();
			}
		}
	}

	return Direction;
}

void ACPPlayerCharacter::PerformAttack()
{
	const FVector AttackDirection = GetAttackDirection();
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
		AttackDuration);

	// A single actor can report multiple hit results (e.g. capsule + mesh), so only
	// process each unique target once per attack.
	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && !HitActors.Contains(HitActor))
		{
			HitActors.Add(HitActor);
			UGameplayStatics::ApplyDamage(HitActor, Stats.AttackPower, GetController(), this, nullptr);
		}
	}
}

void ACPPlayerCharacter::EndDash()
{
	bIsDashing = false;
	bIsInvincible = false;
}

float ACPPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsInvincible)
	{
		return 0.0f;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ACPPlayerCharacter::ApplyStatsToGameplay()
{
	GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed;
}

float ACPPlayerCharacter::GetRequiredExperience() const
{
	return BaseRequiredExperience + static_cast<float>(Stats.Level - 1) * RequiredExperiencePerLevel;
}

void ACPPlayerCharacter::AddExperience(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	Stats.Experience += Amount;

	float RequiredExperience = GetRequiredExperience();
	while (Stats.Experience >= RequiredExperience && RequiredExperience > 0.0f)
	{
		Stats.Experience -= RequiredExperience;
		Stats.Level += 1;

		OnLevelUp.Broadcast(Stats.Level);

		RequiredExperience = GetRequiredExperience();
	}
}

void ACPPlayerCharacter::ModifyStat(ECPStatType StatType, float Delta)
{
	if (StatType == ECPStatType::Experience)
	{
		AddExperience(Delta);
		return;
	}

	SetStat(StatType, GetStat(StatType) + Delta);
}

void ACPPlayerCharacter::SetStat(ECPStatType StatType, float NewValue)
{
	switch (StatType)
	{
	case ECPStatType::Health:
		Stats.Health = NewValue;
		break;
	case ECPStatType::Experience:
		Stats.Experience = NewValue;
		break;
	case ECPStatType::AttackPower:
		Stats.AttackPower = NewValue;
		break;
	case ECPStatType::MoveSpeed:
		Stats.MoveSpeed = NewValue;
		break;
	case ECPStatType::AttackSpeed:
		Stats.AttackSpeed = NewValue;
		break;
	case ECPStatType::Defense:
		Stats.Defense = NewValue;
		break;
	case ECPStatType::Level:
		Stats.Level = FMath::RoundToInt32(NewValue);
		break;
	}

	ApplyStatsToGameplay();
}

float ACPPlayerCharacter::GetStat(ECPStatType StatType) const
{
	switch (StatType)
	{
	case ECPStatType::Health:
		return Stats.Health;
	case ECPStatType::Experience:
		return Stats.Experience;
	case ECPStatType::AttackPower:
		return Stats.AttackPower;
	case ECPStatType::MoveSpeed:
		return Stats.MoveSpeed;
	case ECPStatType::AttackSpeed:
		return Stats.AttackSpeed;
	case ECPStatType::Defense:
		return Stats.Defense;
	case ECPStatType::Level:
		return static_cast<float>(Stats.Level);
	}

	return 0.0f;
}

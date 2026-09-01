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
#include "Player/CPInteractable.h"
#include "Player/CPItemEffect.h"
#include "Weapon/CPWeaponManagerComponent.h"
#include "Weapon/CPWeaponBase.h"

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

	WeaponManager = CreateDefaultSubobject<UCPWeaponManagerComponent>(TEXT("WeaponManager"));
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
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::Interact);
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

void ACPPlayerCharacter::Interact(const FInputActionValue& Value)
{
	DoInteract();
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
	// Armed: the weapon owns its own CanAttack/AttackInterval timing, so just forward the request to it
	if (WeaponManager && WeaponManager->GetCurrentWeapon())
	{
		WeaponManager->Attack();
		return;
	}

	// Unarmed fallback: legacy bare-hand box-trace attack, preserved for backward compatibility
	const float EffectiveAttackCooldown = Stats.AttackSpeed > 0.0f ? (AttackCooldown / Stats.AttackSpeed) : AttackCooldown;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < EffectiveAttackCooldown)
	{
		return;
	}
	LastAttackTime = CurrentTime;

	PerformAttack();
}

ACPWeaponBase* ACPPlayerCharacter::EquipWeapon(TSubclassOf<ACPWeaponBase> WeaponClass)
{
	return WeaponManager ? WeaponManager->EquipWeapon(WeaponClass) : nullptr;
}

ACPWeaponBase* ACPPlayerCharacter::SwapWeapon(TSubclassOf<ACPWeaponBase> NewWeaponClass)
{
	return WeaponManager ? WeaponManager->SwapWeapon(NewWeaponClass) : nullptr;
}

ACPWeaponBase* ACPPlayerCharacter::GetCurrentWeapon() const
{
	return WeaponManager ? WeaponManager->GetCurrentWeapon() : nullptr;
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

void ACPPlayerCharacter::DoInteract()
{
	AActor* Target = CurrentInteractable.Get();
	if (!Target)
	{
		return;
	}

	ICPInteractable* Interactable = Cast<ICPInteractable>(Target);
	if (!Interactable)
	{
		return;
	}

	if (Interactable->CanInteract(this))
	{
		Interactable->Interact(this);
	}
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

	Stats.Experience = FMath::Clamp(Stats.Experience + Amount, ExperienceRange.Min, ExperienceRange.Max);

	float RequiredExperience = GetRequiredExperience();
	while (Stats.Level < FMath::RoundToInt32(LevelRange.Max) && Stats.Experience >= RequiredExperience && RequiredExperience > 0.0f)
	{
		Stats.Experience -= RequiredExperience;
		Stats.Level += 1;

		OnLevelUp.Broadcast(Stats.Level);

		RequiredExperience = GetRequiredExperience();
	}

	Stats.Level = FMath::Clamp(Stats.Level, FMath::RoundToInt32(LevelRange.Min), FMath::RoundToInt32(LevelRange.Max));
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
		Stats.Health = FMath::Clamp(NewValue, HealthRange.Min, HealthRange.Max);
		break;
	case ECPStatType::Experience:
		Stats.Experience = FMath::Clamp(NewValue, ExperienceRange.Min, ExperienceRange.Max);
		break;
	case ECPStatType::AttackPower:
		Stats.AttackPower = FMath::Clamp(NewValue, AttackPowerRange.Min, AttackPowerRange.Max);
		break;
	case ECPStatType::MoveSpeed:
		Stats.MoveSpeed = FMath::Clamp(NewValue, MoveSpeedRange.Min, MoveSpeedRange.Max);
		break;
	case ECPStatType::AttackSpeed:
		Stats.AttackSpeed = FMath::Clamp(NewValue, AttackSpeedRange.Min, AttackSpeedRange.Max);
		break;
	case ECPStatType::Defense:
		Stats.Defense = FMath::Clamp(NewValue, DefenseRange.Min, DefenseRange.Max);
		break;
	case ECPStatType::Level:
		Stats.Level = FMath::Clamp(FMath::RoundToInt32(NewValue), FMath::RoundToInt32(LevelRange.Min), FMath::RoundToInt32(LevelRange.Max));
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

void ACPPlayerCharacter::RegisterInteractable(AActor* Interactable)
{
	if (!Interactable)
	{
		return;
	}

	NearbyInteractables.AddUnique(TWeakObjectPtr<AActor>(Interactable));
	RefreshCurrentInteractable();
}

void ACPPlayerCharacter::UnregisterInteractable(AActor* Interactable)
{
	NearbyInteractables.RemoveAll([Interactable](const TWeakObjectPtr<AActor>& Weak)
	{
		return !Weak.IsValid() || Weak.Get() == Interactable;
	});

	if (CurrentInteractable.Get() == Interactable)
	{
		CurrentInteractable.Reset();
	}

	RefreshCurrentInteractable();
}

void ACPPlayerCharacter::RefreshCurrentInteractable()
{
	NearbyInteractables.RemoveAll([](const TWeakObjectPtr<AActor>& Weak)
	{
		return !Weak.IsValid();
	});

	AActor* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AActor>& Weak : NearbyInteractables)
	{
		AActor* Candidate = Weak.Get();
		if (!Candidate)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Candidate;
		}
	}

	CurrentInteractable = Closest;
}

void ACPPlayerCharacter::AddCoin(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CoinAmount += Amount;
}

int32 ACPPlayerCharacter::GetCoinAmount() const
{
	return CoinAmount;
}

bool ACPPlayerCharacter::HasEnoughCoin(int32 Amount) const
{
	return CoinAmount >= Amount;
}

bool ACPPlayerCharacter::TrySpendCoin(int32 Amount)
{
	if (!HasEnoughCoin(Amount))
	{
		return false;
	}

	CoinAmount -= Amount;
	return true;
}

void ACPPlayerCharacter::AddOwnedItem(const FCPItemData& ItemData)
{
	OwnedItems.Add(ItemData);

	if (ItemData.EffectClass)
	{
		if (UCPItemEffect* Effect = NewObject<UCPItemEffect>(this, ItemData.EffectClass))
		{
			TScriptInterface<ICPStatInterface> StatInterface;
			StatInterface.SetObject(this);
			StatInterface.SetInterface(Cast<ICPStatInterface>(this));

			Effect->ApplyEffect(StatInterface);
		}
	}

	OnItemAcquired.Broadcast(ItemData);
}

bool ACPPlayerCharacter::HasItem(FName ItemCode) const
{
	return OwnedItems.ContainsByPredicate([ItemCode](const FCPItemData& Item)
	{
		return Item.ItemCode == ItemCode;
	});
}

int32 ACPPlayerCharacter::GetItemCount(FName ItemCode) const
{
	int32 Count = 0;

	for (const FCPItemData& Item : OwnedItems)
	{
		if (Item.ItemCode == ItemCode)
		{
			++Count;
		}
	}

	return Count;
}

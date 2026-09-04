// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CPPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
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
#include "Roulette/CPRoulette.h"
#include "Player/CPTopDownPlayerController.h"
#include "Player/CPGameMode.h"

DEFINE_LOG_CATEGORY(LogCPPlayerCharacter);

namespace
{
	constexpr float ReviveDebugDrawInterval = 0.1f;
}

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
	// Bound in the constructor (not BeginPlay) so it's already in place before WeaponManager's own BeginPlay
	// equips DefaultWeaponClass and broadcasts this for the very first weapon
	WeaponManager->OnWeaponChanged.AddDynamic(this, &ACPPlayerCharacter::HandleWeaponChanged);

	ReviveDetectionRange = CreateDefaultSubobject<USphereComponent>(TEXT("ReviveDetectionRange"));
	ReviveDetectionRange->SetupAttachment(RootComponent);
	ReviveDetectionRange->InitSphereRadius(ReviveDetectionRadius);
	ReviveDetectionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	// Radius is re-applied in BeginPlay (see below) so a Blueprint-tuned ReviveDetectionRadius takes
	// effect - InitSphereRadius here only seeds a sane editor-time default
	ReviveDetectionRange->OnComponentBeginOverlap.AddDynamic(this, &ACPPlayerCharacter::OnReviveRangeBeginOverlap);
	ReviveDetectionRange->OnComponentEndOverlap.AddDynamic(this, &ACPPlayerCharacter::OnReviveRangeEndOverlap);
}

void ACPPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyStatsToGameplay();

	ReviveDetectionRange->SetSphereRadius(ReviveDetectionRadius);
	ReviveDetectionRange->ShapeColor = DebugReviveRangeColor;

	if (bDrawDebugReviveRange)
	{
		DrawDebugReviveRangeShape();
		GetWorldTimerManager().SetTimer(DebugReviveRangeTimerHandle, this, &ACPPlayerCharacter::DrawDebugReviveRangeShape, ReviveDebugDrawInterval, true);
	}
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
		EnhancedInputComponent->BindAction(RollRouletteAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::RollRoulette);
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::ToggleCamera);
	}
	else
	{
		UE_LOG(LogCPPlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This character requires the Enhanced Input system."), *GetNameSafe(this));
	}
}

void ACPPlayerCharacter::RollRoulette(const FInputActionValue& Value)
{
	if (!Roulette)
	{
		ACPRoulette* LevelRoulette = Cast<ACPRoulette>(UGameplayStatics::GetActorOfClass(GetWorld(), ACPRoulette::StaticClass()));
		if (!LevelRoulette)
		{
			return;
		}
		Roulette = LevelRoulette;
	}
	if (bIsDowned)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Rollin"));
	Roulette->Roll();
}

void ACPPlayerCharacter::ToggleCamera(const FInputActionValue& Value)
{
	if (ACPGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACPGameMode>() : nullptr)
	{
		GameMode->ToggleCameraMode();
	}
}

void ACPPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Ignore tiny stick drift/noise below the deadzone threshold, so it doesn't register as movement
	// (or, for a gamepad player, as an attack direction - see GetAttackDirection)
	if (MovementVector.Size() < MoveInputDeadzone)
	{
		MovementVector = FVector2D::ZeroVector;
	}

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
	if (Controller == nullptr || bIsAttackLocked || bIsDowned)
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
	if (bIsAttackLocked || bIsDowned)
	{
		return;
	}

	// Armed: the weapon owns its own CanAttack/AttackInterval timing, so just forward the request to it
	if (WeaponManager && WeaponManager->GetCurrentWeapon())
	{
		if (WeaponManager->Attack())
		{
			OrientAndLungeForAttack();
		}
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
	if (bIsDashing || bIsDowned)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDashTime < DashCooldown)
	{
		return;
	}
	LastDashTime = CurrentTime;

	// Dash interrupts an in-progress attack: cancel the weapon's swing (which releases the attack lock via
	// HandleAttackStateChanged) so the dash below isn't blocked and doesn't fight the attack's facing/lunge
	if (bIsAttackLocked)
	{
		if (ACPWeaponBase* CurrentWeapon = WeaponManager ? WeaponManager->GetCurrentWeapon() : nullptr)
		{
			CurrentWeapon->CancelAttack();
		}
	}

	DashDirection = GetLastMovementWorldDirection();

	bIsDashing = true;
	bIsInvincible = true;

	const float DashSpeed = DashDuration > 0.0f ? (DashDistance / DashDuration) : DashDistance;
	LaunchCharacter(DashDirection * DashSpeed, true, true);

	GetWorldTimerManager().SetTimer(DashDurationTimerHandle, this, &ACPPlayerCharacter::EndDash, DashDuration, false);
}

void ACPPlayerCharacter::HandleWeaponChanged(ACPWeaponBase* NewWeapon)
{
	if (NewWeapon)
	{
		NewWeapon->OnAttackStateChanged.AddDynamic(this, &ACPPlayerCharacter::HandleAttackStateChanged);
	}
}

void ACPPlayerCharacter::HandleAttackStateChanged(bool bIsAttacking)
{
	bIsAttackLocked = bIsAttacking;

	// Movement input is already blocked outright while locked (see DoMove) - this additionally stops the
	// movement component from turning the character to face residual velocity while the lock is engaged
	GetCharacterMovement()->bOrientRotationToMovement = !bIsAttacking;
}

void ACPPlayerCharacter::DoInteract()
{
	if (bIsDowned)
	{
		return;
	}

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

FVector ACPPlayerCharacter::GetLastMovementWorldDirection() const
{
	FVector Direction = GetWorldDirectionFromInput(LastMoveInputVector);
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	return Direction;
}

FVector ACPPlayerCharacter::GetAttackDirection() const
{
	// Gamepad player: attack in the current/last movement direction instead of aiming with a mouse
	// cursor (a gamepad player has no meaningful cursor position, and there's no separate aim stick)
	if (const ACPTopDownPlayerController* TopDownController = Cast<ACPTopDownPlayerController>(GetController()))
	{
		if (!TopDownController->IsUsingKeyboardAndMouse())
		{
			return GetLastMovementWorldDirection();
		}
	}

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

void ACPPlayerCharacter::OrientAndLungeForAttack()
{
	const FVector AimDirection = GetAttackDirection();
	SetActorRotation(FRotator(0.0f, AimDirection.Rotation().Yaw, 0.0f));

	const FVector CurrentVelocity = GetVelocity();
	const float CurrentSpeed = CurrentVelocity.Size2D();
	if (CurrentSpeed < AttackLungeMinSpeed)
	{
		return;
	}

	const FVector LungeDirection = CurrentVelocity.GetSafeNormal2D();
	const float LungeSpeed = AttackLungeDuration > 0.0f ? (AttackLungeDistance / AttackLungeDuration) : AttackLungeDistance;
	LaunchCharacter(LungeDirection * LungeSpeed, true, true);
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
	if (bIsInvincible || bIsDowned)
	{
		return 0.0f;
	}

	SetStat(ECPStatType::Health, GetStat(ECPStatType::Health) - DamageAmount);

	if (GetStat(ECPStatType::Health) <= 0.0f)
	{
		EnterDownedState();
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ACPPlayerCharacter::ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor)
{
	if (bIsInvincible || bIsDowned)
	{
		return;
	}

	ApplyCPKnockbackToCharacter(this, Direction, Distance, KnockbackDuration, KnockbackLaunchStrength);
}

void ACPPlayerCharacter::OnReviveRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsDowned || CurrentReviver.IsValid())
	{
		return;
	}

	if (!OtherActor || OtherActor == this || !OtherActor->IsA<ACPPlayerCharacter>())
	{
		return;
	}

	StartRevive(OtherActor);
}

void ACPPlayerCharacter::OnReviveRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor != CurrentReviver.Get())
	{
		return;
	}

	// Leaving the revive area resets progress entirely, rather than pausing it
	GetWorldTimerManager().ClearTimer(ReviveTimerHandle);
	CurrentReviver.Reset();
	ReviveStartTime = -1.0f;
}

void ACPPlayerCharacter::EnterDownedState()
{
	if (bIsDowned)
	{
		return;
	}

	bIsDowned = true;

	// Interrupt whatever the character was doing the moment it went down
	if (ACPWeaponBase* CurrentWeapon = WeaponManager ? WeaponManager->GetCurrentWeapon() : nullptr)
	{
		CurrentWeapon->CancelAttack();
	}
	if (bIsDashing)
	{
		GetWorldTimerManager().ClearTimer(DashDurationTimerHandle);
		EndDash();
	}

	GetCharacterMovement()->DisableMovement();
	ReviveDetectionRange->ShapeColor = FColor::Red;

	OnPlayerDowned.Broadcast();

	// A reviver may already be standing in range when this character goes down - OnComponentBeginOverlap
	// won't re-fire for an already-overlapping actor, so check for one explicitly
	TArray<AActor*> OverlappingActors;
	ReviveDetectionRange->GetOverlappingActors(OverlappingActors, ACPPlayerCharacter::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor && OverlappingActor != this)
		{
			StartRevive(OverlappingActor);
			break;
		}
	}
}

void ACPPlayerCharacter::StartRevive(AActor* Reviver)
{
	CurrentReviver = Reviver;
	ReviveStartTime = GetWorld()->GetTimeSeconds();

	GetWorldTimerManager().SetTimer(ReviveTimerHandle, this, &ACPPlayerCharacter::Revive, ReviveDuration, false);
}

void ACPPlayerCharacter::Revive()
{
	bIsDowned = false;
	CurrentReviver.Reset();
	ReviveStartTime = -1.0f;
	GetWorldTimerManager().ClearTimer(ReviveTimerHandle);

	ReviveDetectionRange->ShapeColor = DebugReviveRangeColor;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetStat(ECPStatType::Health, HealthRange.Max * ReviveHealthPercent);

	OnPlayerRevived.Broadcast();
}

float ACPPlayerCharacter::GetReviveTimeRemaining() const
{
	if (!bIsDowned || ReviveStartTime < 0.0f)
	{
		return 0.0f;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - ReviveStartTime;
	return FMath::Max(ReviveDuration - Elapsed, 0.0f);
}

void ACPPlayerCharacter::DrawDebugReviveRangeShape() const
{
	if (!ReviveDetectionRange)
	{
		return;
	}

	const FColor SphereColor = bIsDowned ? FColor::Red : DebugReviveRangeColor;
	DrawDebugSphere(GetWorld(), ReviveDetectionRange->GetComponentLocation(), ReviveDetectionRange->GetScaledSphereRadius(), 16, SphereColor, false, ReviveDebugDrawInterval * 1.5f, 0, 1.0f);
}

void ACPPlayerCharacter::ApplyStatsToGameplay()
{
	GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed;
}

void ACPPlayerCharacter::ModifyStat(ECPStatType StatType, float Delta)
{
	SetStat(StatType, GetStat(StatType) + Delta);
}

void ACPPlayerCharacter::SetStat(ECPStatType StatType, float NewValue)
{
	switch (StatType)
	{
	case ECPStatType::Health:
		Stats.Health = FMath::Clamp(NewValue, HealthRange.Min, HealthRange.Max);
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
	default:
		// Experience/Level are owned by ACPGameMode now (team-shared) - not tracked per player
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
	case ECPStatType::AttackPower:
		return Stats.AttackPower;
	case ECPStatType::MoveSpeed:
		return Stats.MoveSpeed;
	case ECPStatType::AttackSpeed:
		return Stats.AttackSpeed;
	case ECPStatType::Defense:
		return Stats.Defense;
	default:
		// Experience/Level are owned by ACPGameMode now (team-shared) - not tracked per player
		break;
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

	NotifyItemAcquired(ItemData);
}

void ACPPlayerCharacter::NotifyItemAcquired(const FCPItemData& ItemData)
{
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

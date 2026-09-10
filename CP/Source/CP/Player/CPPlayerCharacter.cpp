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
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "KismetAnimationLibrary.h"
#include "TimerManager.h"
#include "Player/CPInteractable.h"
#include "Player/CPItemEffect.h"
#include "Weapon/CPWeaponManagerComponent.h"
#include "Weapon/CPWeaponBase.h"
#include "Roulette/CPRoulette.h"
#include "UI/CPRadialGaugeComponent.h"
#include "Debug/CPDebugCollisionSubsystem.h"
#include "Debug/CPDebugCollisionShapeComponent.h"
#include "Monster/Spawner/CPMonsterSpawnManagerComponent.h"
#include "Player/Stat/CPPlayerStatTableTypes.h"
#include "Player/Inventory/CPInventoryComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/MeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraShakeBase.h"

DEFINE_LOG_CATEGORY(LogCPPlayerCharacter);

namespace
{
	constexpr float ReviveDebugDrawInterval = 0.1f;
	constexpr float ReviveGaugeUpdateInterval = 0.1f;
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

	DebugHitboxShape = CreateDefaultSubobject<UCPDebugCollisionShapeComponent>(TEXT("DebugHitboxShape"));
	DebugHitboxShape->Category = ECPDebugCollisionCategory::PlayerHitbox;
	DebugHitboxShape->SetTargetComponent(GetCapsuleComponent());

	// kohMS
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel7, ECR_Overlap);

	MonsterSpawnManager = CreateDefaultSubobject<UCPMonsterSpawnManagerComponent>(TEXT("MonsterSpawnManager"));
	HitFlashTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("HitFlashTimeline"));

	UCurveFloat* DefaultHitFlashCurve = CreateDefaultSubobject<UCurveFloat>(TEXT("HitFlashDefaultCurve"));
	DefaultHitFlashCurve->FloatCurve.AddKey(0.0f, 1.0f);
	DefaultHitFlashCurve->FloatCurve.AddKey(1.0f, 0.0f);
	HitFlashCurve = DefaultHitFlashCurve;

	InventoryComponent = CreateDefaultSubobject<UCPInventoryComponent>(TEXT("InventoryComponent"));
}

void ACPPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitStatsFromDataTable();
	ApplyStatsToGameplay();

	TArray<UMeshComponent*> PlayerMeshComponents;
	GetComponents<UMeshComponent>(PlayerMeshComponents);
	for (UMeshComponent* PlayerMeshComponent : PlayerMeshComponents)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < PlayerMeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			if (UMaterialInstanceDynamic* MID = PlayerMeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex))
			{
				HitFlashMIDs.Add(MID);
			}
		}
	}

	if (HitFlashTimeline && HitFlashCurve)
	{
		FOnTimelineFloat HitFlashUpdateEvent;
		HitFlashUpdateEvent.BindUFunction(this, FName("HandleHitFlashUpdate"));
		HitFlashTimeline->AddInterpFloat(HitFlashCurve, HitFlashUpdateEvent);
		HitFlashTimeline->SetLooping(false);
		HitFlashTimeline->SetPlayRate(HitFlashSpeed);
	}

	ReviveDetectionRange->SetSphereRadius(ReviveDetectionRadius);
	ReviveDetectionRange->ShapeColor = DebugReviveRangeColor;

	if (UCPDebugCollisionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UCPDebugCollisionSubsystem>() : nullptr)
	{
		Subsystem->OnCollisionVisibilityChanged.AddDynamic(this, &ACPPlayerCharacter::HandleDebugCollisionVisibilityChanged);
		SetReviveRangeDebugDrawEnabled(Subsystem->IsCategoryVisible(ECPDebugCollisionCategory::PlayerRevive));
	}
	else if (bDrawDebugReviveRange)
	{
		DrawDebugReviveRangeShape();
		GetWorldTimerManager().SetTimer(DebugReviveRangeTimerHandle, this, &ACPPlayerCharacter::DrawDebugReviveRangeShape, ReviveDebugDrawInterval, true);
	}
}

void ACPPlayerCharacter::HandleDebugCollisionVisibilityChanged(ECPDebugCollisionCategory Category, bool bVisible)
{
	if (Category == ECPDebugCollisionCategory::PlayerRevive)
	{
		SetReviveRangeDebugDrawEnabled(bVisible);
	}
}

void ACPPlayerCharacter::SetReviveRangeDebugDrawEnabled(bool bEnabled)
{
	bDrawDebugReviveRange = bEnabled;
	GetWorldTimerManager().ClearTimer(DebugReviveRangeTimerHandle);

	if (bEnabled)
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
		EnhancedInputComponent->BindAction(UseSlotEastAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::UseSlotEast);
		EnhancedInputComponent->BindAction(UseSlotNorthAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::UseSlotNorth);
		EnhancedInputComponent->BindAction(UseSlotWestAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::UseSlotWest);
		EnhancedInputComponent->BindAction(UseSlotSouthAction, ETriggerEvent::Started, this, &ACPPlayerCharacter::UseSlotSouth);
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

void ACPPlayerCharacter::UseSlotEast(const FInputActionValue& Value)
{
	if (InventoryComponent)
	{
		InventoryComponent->UseSlotItem(0);
	}
}

void ACPPlayerCharacter::UseSlotNorth(const FInputActionValue& Value)
{
	if (InventoryComponent)
	{
		InventoryComponent->UseSlotItem(1);
	}
}

void ACPPlayerCharacter::UseSlotWest(const FInputActionValue& Value)
{
	if (InventoryComponent)
	{
		InventoryComponent->UseSlotItem(2);
	}
}

void ACPPlayerCharacter::UseSlotSouth(const FInputActionValue& Value)
{
	if (InventoryComponent)
	{
		InventoryComponent->UseSlotItem(3);
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
	if (Controller == nullptr || bIsDowned)
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
	if (bIsDowned)
	{
		return;
	}

	// ACPWeaponBase::CanAttack() already no-ops this while a combo string is in progress - no need to also
	// gate on bIsAttackLocked here. Facing (see OrientTowardsAttackDirection) is handled by
	// HandleAttackStateChanged, triggered by the weapon's own OnAttackStateChanged the moment this actually
	// starts a new attack
	if (WeaponManager && WeaponManager->GetCurrentWeapon())
	{
		WeaponManager->Attack();
	}
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
	// HandleAttackStateChanged) so the dash below doesn't fight the attack's facing lock
	if (bIsAttackLocked)
	{
		if (ACPWeaponBase* CurrentWeapon = WeaponManager ? WeaponManager->GetCurrentWeapon() : nullptr)
		{
			CurrentWeapon->CancelAttack();
		}

		// CancelAttack's HandleAttackStateChanged(false) would normally wait PostAttackRotationDelay before
		// resuming movement-driven rotation - a dash is a decisive movement action though, so resume right
		// away instead of leaving the character facing the old attack direction through the dash
		GetWorldTimerManager().ClearTimer(PostAttackRotationTimerHandle);
		ReorientToMovementDirection();
	}

	DashDirection = GetLastMovementWorldDirection();

	bIsDashing = true;
	BeginInvincibility();

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

	if (bIsAttacking)
	{
		// A new combo string just started: stop the movement component from turning the character to face
		// movement, and snap to face the attack direction instead. Movement itself is left alone - the
		// player can keep moving freely, just without the character turning to follow it
		GetWorldTimerManager().ClearTimer(PostAttackRotationTimerHandle);
		GetCharacterMovement()->bOrientRotationToMovement = false;
		OrientTowardsAttackDirection();
	}
	else
	{
		// The attack's motion just ended (montage finished, or last swing dispatched if no montage) - keep
		// facing the attack direction for a bit longer before resuming movement-driven rotation
		GetWorldTimerManager().SetTimer(PostAttackRotationTimerHandle, this, &ACPPlayerCharacter::ReorientToMovementDirection, PostAttackRotationDelay, false);
	}
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

float ACPPlayerCharacter::GetMovementDirection() const
{
	return UKismetAnimationLibrary::CalculateDirection(GetVelocity(), GetActorRotation());
}

FVector ACPPlayerCharacter::GetAttackDirection() const
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return GetActorForwardVector();
	}

	// Checked live every time, instead of asking "which device does this player own" (that's a per-player
	// classification meant for local multiplayer/the lobby - it's meaningless when testing this level
	// directly, and it forces an either/or choice). This way the same controller can freely mix mouse
	// aiming and gamepad-stick aiming from one attack to the next
	if (PC->PlayerInput)
	{
		const float GamepadX = PC->PlayerInput->GetKeyValue(EKeys::Gamepad_LeftX);
		const float GamepadY = PC->PlayerInput->GetKeyValue(EKeys::Gamepad_LeftY);
		if (FVector2D(GamepadX, GamepadY).Size() >= MoveInputDeadzone)
		{
			return GetLastMovementWorldDirection();
		}
	}

	// Mouse: intersect the deprojected cursor ray with a horizontal plane at the character's own height,
	// instead of GetHitResultUnderCursor - that depends on the floor mesh actually blocking ECC_Visibility
	// (or nothing else unexpectedly blocking it first), and silently falls back to "just face forward" the
	// moment it doesn't. This plane intersection is pure math, so it can't fail because of level collision setup
	FVector RayOrigin, RayDirection;
	if (PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) && !FMath::IsNearlyZero(RayDirection.Z))
	{
		const float CharacterZ = GetActorLocation().Z;
		const float DistanceToPlane = (CharacterZ - RayOrigin.Z) / RayDirection.Z;

		if (DistanceToPlane > 0.0f)
		{
			FVector ToCursor = (RayOrigin + RayDirection * DistanceToPlane) - GetActorLocation();
			ToCursor.Z = 0.0f;

			if (!ToCursor.IsNearlyZero())
			{
				return ToCursor.GetSafeNormal();
			}
		}
	}

	return GetActorForwardVector();
}

void ACPPlayerCharacter::OrientTowardsAttackDirection()
{
	const FVector AimDirection = GetAttackDirection();
	SetActorRotation(FRotator(0.0f, AimDirection.Rotation().Yaw, 0.0f));
}

void ACPPlayerCharacter::ReorientToMovementDirection()
{
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void ACPPlayerCharacter::EndDash()
{
	bIsDashing = false;
	EndInvincibilityRequest();
}

void ACPPlayerCharacter::BeginInvincibility()
{
	++InvincibilityRequestCount;
}

void ACPPlayerCharacter::EndInvincibilityRequest()
{
	InvincibilityRequestCount = FMath::Max(InvincibilityRequestCount - 1, 0);
}

void ACPPlayerCharacter::SetDebugInvincible(bool bEnabled)
{
	if (bIsDebugInvincible == bEnabled)
	{
		return;
	}

	bIsDebugInvincible = bEnabled;

	if (bEnabled)
	{
		BeginInvincibility();
	}
	else
	{
		EndInvincibilityRequest();
	}
}

float ACPPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsInvincible() || bIsDowned)
	{
		return 0.0f;
	}

	PlayHitFlash();
	PlayHitCameraShake();

	SetStat(ECPStatType::Health, GetStat(ECPStatType::Health) - DamageAmount);

	if (GetStat(ECPStatType::Health) <= 0.0f)
	{
		EnterDownedState();
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ACPPlayerCharacter::ApplyKnockback(const FVector& Direction, float Distance, AActor* InstigatorActor)
{
	if (IsInvincible() || bIsDowned)
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

	GetWorldTimerManager().ClearTimer(ReviveGaugeUpdateTimerHandle);
	if (ReviveGaugeComponent)
	{
		ReviveGaugeComponent->SetGaugeEnabled(false);
	}
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

	if (ReviveGaugeComponent)
	{
		ReviveGaugeComponent->SetGaugeEnabled(true);
		UpdateReviveGaugeDisplay();
		GetWorldTimerManager().SetTimer(ReviveGaugeUpdateTimerHandle, this, &ACPPlayerCharacter::UpdateReviveGaugeDisplay, ReviveGaugeUpdateInterval, true);
	}
}

void ACPPlayerCharacter::Revive()
{
	bIsDowned = false;
	CurrentReviver.Reset();
	ReviveStartTime = -1.0f;
	GetWorldTimerManager().ClearTimer(ReviveTimerHandle);

	GetWorldTimerManager().ClearTimer(ReviveGaugeUpdateTimerHandle);
	if (ReviveGaugeComponent)
	{
		ReviveGaugeComponent->SetGaugeEnabled(false);
	}

	ReviveDetectionRange->ShapeColor = DebugReviveRangeColor;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetStat(ECPStatType::Health, HealthRange.Max * ReviveHealthPercent);

	// Brief invincibility window right after coming back up, so a nearby monster can't immediately
	// down the character again before they can react
	if (PostReviveInvincibilityDuration > 0.0f)
	{
		BeginInvincibility();
		GetWorldTimerManager().SetTimer(PostReviveInvincibilityTimerHandle, this, &ACPPlayerCharacter::EndInvincibilityRequest, PostReviveInvincibilityDuration, false);
	}

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

void ACPPlayerCharacter::UpdateReviveGaugeDisplay()
{
	if (!ReviveGaugeComponent)
	{
		return;
	}

	const float Elapsed = ReviveDuration - GetReviveTimeRemaining();
	ReviveGaugeComponent->UpdateGauge(Elapsed, ReviveDuration);
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

void ACPPlayerCharacter::PlayHitFlash()
{
	if (!HitFlashTimeline)
	{
		return;
	}

	for (UMaterialInstanceDynamic* MID : HitFlashMIDs)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(HitFlashColorParameterName, HitFlashColor);
		}
	}

	HitFlashTimeline->SetPlayRate(HitFlashSpeed);
	HitFlashTimeline->PlayFromStart();
}

void ACPPlayerCharacter::HandleHitFlashUpdate(float Value)
{
	for (UMaterialInstanceDynamic* MID : HitFlashMIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(HitFlashAmountParameterName, Value);
		}
	}
}

void ACPPlayerCharacter::PlayHitCameraShake()
{
	if (!HitCameraShakeClass)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ClientStartCameraShake(HitCameraShakeClass, HitCameraShakeIntensity);
	}
}

void ACPPlayerCharacter::InitStatsFromDataTable()
{
	if (!BaseStatTable)
	{
		return;
	}

	static const FName RowName(TEXT("Default"));
	if (const FCPPlayerBaseStatRow* Row = BaseStatTable->FindRow<FCPPlayerBaseStatRow>(RowName, TEXT("InitStatsFromDataTable")))
	{
		HealthRange = FCPStatRange(Row->HealthMin, Row->HealthMax);
		AttackPowerRange = FCPStatRange(Row->AttackPowerMin, Row->AttackPowerMax);
		MoveSpeedRange = FCPStatRange(Row->MoveSpeedMin, Row->MoveSpeedMax);
		AttackSpeedRange = FCPStatRange(Row->AttackSpeedMin, Row->AttackSpeedMax);

		Stats.Health = Row->Health;
		Stats.AttackPower = Row->AttackPower;
		Stats.MoveSpeed = Row->MoveSpeed;
		Stats.AttackSpeed = Row->AttackSpeed;
	}
}

const FCPPlayerLevelStatRow* ACPPlayerCharacter::FindLevelStatRow(int32 InLevel) const
{
	if (!LevelStatTable)
	{
		return nullptr;
	}

	const FName RowName(*FString::FromInt(InLevel));
	return LevelStatTable->FindRow<FCPPlayerLevelStatRow>(RowName, TEXT("FindLevelStatRow"), false);
}

float ACPPlayerCharacter::GetRequiredExperienceForLevel(int32 InLevel) const
{
	if (const FCPPlayerLevelStatRow* Row = FindLevelStatRow(InLevel))
	{
		return Row->RequiredExperience;
	}

	return ExperienceRange.Max;
}

void ACPPlayerCharacter::AddCoin(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CoinCount += Amount;

	OnCoinChanged.Broadcast(CoinCount);
}

bool ACPPlayerCharacter::TrySpendCoin(int32 Amount)
{
	if (!HasEnoughCoin(Amount))
	{
		return false;
	}

	CoinCount -= Amount;

	OnCoinChanged.Broadcast(CoinCount);

	return true;
}

void ACPPlayerCharacter::AddTicket(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	TicketCount += Amount;

	OnTicketChanged.Broadcast(TicketCount);
}

bool ACPPlayerCharacter::TrySpendTicket(int32 Amount)
{
	if (Amount <= 0 || TicketCount < Amount)
	{
		return false;
	}

	TicketCount -= Amount;

	OnTicketChanged.Broadcast(TicketCount);

	return true;
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
		OnHealthChanged.Broadcast(Stats.Health, HealthRange.Max);
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
	case ECPStatType::Experience:
	{
		float RemainingExperience = FMath::Max(NewValue, 0.0f);
		float RequiredExperience = GetRequiredExperienceForLevel(Stats.Level);
		while (RequiredExperience > 0.0f && RemainingExperience >= RequiredExperience)
		{
			RemainingExperience -= RequiredExperience;
			++Stats.Level;

			if (const FCPPlayerLevelStatRow* LevelRow = FindLevelStatRow(Stats.Level))
			{
				HealthRange.Max += LevelRow->AddHealth;
				Stats.Health += LevelRow->AddHealth;
			}

			RequiredExperience = GetRequiredExperienceForLevel(Stats.Level);
		}
		Stats.Experience = RemainingExperience;
		break;
	}
	case ECPStatType::Level:
		Stats.Level = FMath::Max(FMath::RoundToInt(NewValue), 1);
		break;
	default:
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
	case ECPStatType::Experience:
		return Stats.Experience;
	case ECPStatType::Level:
		return static_cast<float>(Stats.Level);
	default:
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

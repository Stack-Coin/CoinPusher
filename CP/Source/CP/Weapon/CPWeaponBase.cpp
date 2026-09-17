// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPAimDirectionInterface.h"
#include "Weapon/CPWeaponPassiveSkillModule.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Player/CPStatInterface.h"
#include "Player/CPStatTypes.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

ACPWeaponBase::ACPWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponRoot);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	MovementEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MovementEffectComponent"));
	MovementEffectComponent->SetupAttachment(WeaponMesh);
	MovementEffectComponent->bAutoActivate = false;
}

void ACPWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (MovementEffectComponent)
	{
		MovementEffectComponent->SetRelativeLocation(MovementEffectLocationOffset);
		MovementEffectComponent->SetRelativeRotation(MovementEffectRotationOffset);
		MovementEffectComponent->SetRelativeScale3D(MovementEffectScale);

		if (MovementEffect)
		{
			MovementEffectComponent->SetAsset(MovementEffect);
		}
	}
}

void ACPWeaponBase::EquipTo(ACharacter* NewOwner, FName SocketName)
{
	if (!NewOwner)
	{
		return;
	}

	SetOwner(NewOwner);

	if (USkeletalMeshComponent* OwnerMesh = NewOwner->GetMesh())
	{
		AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	}

	bIsAttacking = false;
	CurrentComboIndex = 0;
}

void ACPWeaponBase::Unequip()
{
	GetWorldTimerManager().ClearTimer(ComboTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackIntervalTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEffectTimerHandle);
	GetWorldTimerManager().ClearTimer(PassiveStatBuffTimerHandle);
	ClearPassiveStatBuff();
	bIsAttacking = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

bool ACPWeaponBase::CanAttack() const
{
	return !bIsAttacking;
}

void ACPWeaponBase::Attack()
{
	if (!CanAttack())
	{
		return;
	}

	StartAttack();
}

void ACPWeaponBase::StartAttack()
{
	bIsAttacking = true;
	CurrentComboIndex = 0;
	CapturedAttackDirection = ResolveAimDirection();
	bWaitingForMontageEnd = false;
	++AttackSessionId;

	if (AttackMontage)
	{
		if (ACharacter* OwnerCharacter = GetOwningCharacter())
		{
			const float MontageLength = OwnerCharacter->PlayAnimMontage(AttackMontage, 1.0f); //GetFinalAttackSpeed()
			USkeletalMeshComponent* OwnerMesh = MontageLength > 0.0f ? OwnerCharacter->GetMesh() : nullptr;
			UAnimInstance* AnimInstance = OwnerMesh ? OwnerMesh->GetAnimInstance() : nullptr;

			if (AnimInstance)
			{
				bWaitingForMontageEnd = true;

				const int32 ThisAttackSessionId = AttackSessionId;
				FOnMontageEnded EndDelegate;
				EndDelegate.BindLambda([this, ThisAttackSessionId](UAnimMontage* /*Montage*/, bool /*bInterrupted*/)
				{
					// Ignore a late end/interrupt notification from an attack that was since cancelled and
					// a new one started (AttackSessionId would have moved on by then)
					if (ThisAttackSessionId == AttackSessionId)
					{
						bWaitingForMontageEnd = false;
						OnAttackStateChanged.Broadcast(false);
					}
				});
				AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);
			}
		}
	}

	OnAttackStateChanged.Broadcast(true);

	PerformComboStep();
}

void ACPWeaponBase::CancelAttack()
{
	if (!bIsAttacking)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ComboTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackIntervalTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEffectTimerHandle);
	bIsAttacking = false;

	if (AttackMontage)
	{
		if (ACharacter* OwnerCharacter = GetOwningCharacter())
		{
			OwnerCharacter->StopAnimMontage(AttackMontage);
		}
	}

	OnAttackStateChanged.Broadcast(false);
}

void ACPWeaponBase::PerformComboStep()
{
	ExecuteAttack(CurrentComboIndex);
	++CurrentComboIndex;

	if (CurrentComboIndex < WeaponData.AttackCount)
	{
		GetWorldTimerManager().SetTimer(ComboTimerHandle, this, &ACPWeaponBase::PerformComboStep, FMath::Max(WeaponData.ComboAttackInterval, 0.0f), false);
	}
	else
	{
		// The combo's last swing has just been dispatched. If an attack montage is playing, StartAttack's
		// end-delegate releases the lock once it's actually done instead - otherwise (no montage assigned)
		// release it now rather than waiting out AttackInterval below, which only rate-limits the next
		// Attack() call and isn't part of the swing itself
		if (!bWaitingForMontageEnd)
		{
			OnAttackStateChanged.Broadcast(false);
		}

		GetWorldTimerManager().SetTimer(AttackIntervalTimerHandle, this, &ACPWeaponBase::FinishAttack, GetFinalAttackInterval(), false);
	}
}

void ACPWeaponBase::FinishAttack()
{
	bIsAttacking = false;
}

float ACPWeaponBase::GetFinalAttackPower() const
{
	float WielderAttackPower = 0.0f;
	if (const ICPStatInterface* StatInterface = GetOwnerStatInterface())
	{
		WielderAttackPower = StatInterface->GetStat(ECPStatType::AttackPower);
	}

	return WielderAttackPower + WeaponData.AttackPower + PassiveAttackPowerBonus;
}

float ACPWeaponBase::GetFinalAttackSpeed() const
{
	float WielderAttackSpeed = 1.0f;
	if (const ICPStatInterface* StatInterface = GetOwnerStatInterface())
	{
		WielderAttackSpeed = StatInterface->GetStat(ECPStatType::AttackSpeed);
	}

	return FMath::Max(WielderAttackSpeed * WeaponData.AttackSpeed * PassiveAttackSpeedMultiplier, KINDA_SMALL_NUMBER);
}

float ACPWeaponBase::GetFinalAttackInterval() const
{
	return FMath::Max(WeaponData.AttackInterval / GetFinalAttackSpeed(), MinAttackInterval);
}

ICPStatInterface* ACPWeaponBase::GetOwnerStatInterface() const
{
	return Cast<ICPStatInterface>(GetOwner());
}

ACharacter* ACPWeaponBase::GetOwningCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

FVector ACPWeaponBase::ResolveAimDirection() const
{
	if (ACharacter* OwnerCharacter = GetOwningCharacter())
	{
		if (const ICPAimDirectionProvider* AimProvider = Cast<ICPAimDirectionProvider>(OwnerCharacter))
		{
			return AimProvider->GetAimDirection();
		}

		return OwnerCharacter->GetActorForwardVector();
	}

	return GetActorForwardVector();
}

void ACPWeaponBase::ActivatePassiveSkill()
{
	if (!PassiveSkillModule)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACPWeaponBase::ActivatePassiveSkill - '%s' has no PassiveSkillModule assigned"), *GetNameSafe(this));
		return;
	}

	ACharacter* OwnerCharacter = GetOwningCharacter();

	FCPPassiveSkillActivationContext Context;
	Context.Weapon = this;
	Context.OwnerCharacter = OwnerCharacter;
	Context.InstigatorController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
	Context.DamageCauser = OwnerCharacter ? static_cast<AActor*>(OwnerCharacter) : static_cast<AActor*>(this);
	Context.Origin = OwnerCharacter ? OwnerCharacter->GetActorLocation() : GetActorLocation();

	PassiveSkillModule->Activate(Context);
}

int32 ACPWeaponBase::GetMaxWeaponLevel() const
{
	return PassiveSkillModule ? PassiveSkillModule->GetMaxLevel() : 1;
}

bool ACPWeaponBase::SetWeaponLevel(int32 NewLevel)
{
	const int32 ClampedLevel = FMath::Clamp(NewLevel, 1, GetMaxWeaponLevel());
	if (ClampedLevel == WeaponLevel)
	{
		return false;
	}

	WeaponLevel = ClampedLevel;
	return true;
}

bool ACPWeaponBase::LevelUp()
{
	return SetWeaponLevel(WeaponLevel + 1);
}

void ACPWeaponBase::ApplyPassiveStatBuff(float InAttackPowerBonus, float InAttackSpeedMultiplierBonus, float InRangeMultiplierBonus, float InDuration,
	UNiagaraSystem* InBuffEffect, const FVector& InBuffEffectLocationOffset, const FRotator& InBuffEffectRotationOffset, const FVector& InBuffEffectScale)
{
	PassiveAttackPowerBonus = InAttackPowerBonus;
	PassiveAttackSpeedMultiplier = 1.0f + InAttackSpeedMultiplierBonus;
	PassiveRangeMultiplier = 1.0f + InRangeMultiplierBonus;
	PassiveStatBuffDuration = FMath::Max(InDuration, 0.01f);

	GetWorldTimerManager().SetTimer(PassiveStatBuffTimerHandle, this, &ACPWeaponBase::ClearPassiveStatBuff, PassiveStatBuffDuration, false);

	PassiveBuffAttackEffect = InBuffEffect;
	PassiveBuffAttackEffectLocationOffset = InBuffEffectLocationOffset;
	PassiveBuffAttackEffectRotationOffset = InBuffEffectRotationOffset;
	PassiveBuffAttackEffectScale = InBuffEffectScale;
}

void ACPWeaponBase::ClearPassiveStatBuff()
{
	PassiveAttackPowerBonus = 0.0f;
	PassiveAttackSpeedMultiplier = 1.0f;
	PassiveRangeMultiplier = 1.0f;

	PassiveBuffAttackEffect = nullptr;
	PassiveBuffAttackEffectLocationOffset = FVector::ZeroVector;
	PassiveBuffAttackEffectRotationOffset = FRotator::ZeroRotator;
	PassiveBuffAttackEffectScale = FVector(1.0f, 1.0f, 1.0f);
}

float ACPWeaponBase::GetPassiveStatBuffTimeRemaining() const
{
	return GetWorldTimerManager().GetTimerRemaining(PassiveStatBuffTimerHandle);
}

void ACPWeaponBase::SetMovementEffectActive(bool bActive)
{
	if (!MovementEffectComponent || !MovementEffect || bActive == bMovementEffectActive)
	{
		return;
	}

	bMovementEffectActive = bActive;

	if (bActive)
	{
		MovementEffectComponent->Activate(true);
	}
	else
	{
		MovementEffectComponent->Deactivate();
	}
}

void ACPWeaponBase::PlayAttackEffect(const FVector& Location, const FRotator& Rotation) const
{
	UNiagaraSystem* EffectToPlay = WeaponData.AttackEffect;
	FVector LocationOffset = WeaponData.AttackEffectLocationOffset;
	FRotator RotationOffset = WeaponData.AttackEffectRotationOffset;
	FVector Scale = WeaponData.AttackEffectScale;

	if (PassiveBuffAttackEffect)
	{
		EffectToPlay = PassiveBuffAttackEffect;
		LocationOffset = PassiveBuffAttackEffectLocationOffset;
		RotationOffset = PassiveBuffAttackEffectRotationOffset;
		Scale = PassiveBuffAttackEffectScale;
	}

	if (EffectToPlay)
	{
		const FVector FinalLocation = Location + Rotation.RotateVector(LocationOffset);
		const FRotator FinalRotation = Rotation + RotationOffset;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, EffectToPlay, FinalLocation, FinalRotation, Scale);
	}
}

void ACPWeaponBase::PlayAttackSound(const FVector& Location, const FRotator& Rotation) const
{
	if (WeaponData.AttackSound)
	{
		const FVector FinalLocation = Location + Rotation.RotateVector(WeaponData.AttackSoundLocationOffset);
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData.AttackSound, FinalLocation, WeaponData.AttackSoundVolume);
	}
}

void ACPWeaponBase::TriggerAttackEffect(const FVector& Location, const FRotator& Rotation)
{
	GetWorldTimerManager().ClearTimer(AttackEffectTimerHandle);

	if (WeaponData.AttackEffectDelay > 0.0f)
	{
		PendingAttackEffectLocation = Location;
		PendingAttackEffectRotation = Rotation;
		GetWorldTimerManager().SetTimer(AttackEffectTimerHandle, this, &ACPWeaponBase::PlayPendingAttackEffect, WeaponData.AttackEffectDelay, false);
	}
	else
	{
		PlayAttackEffect(Location, Rotation);
		PlayAttackSound(Location, Rotation);
	}
}

void ACPWeaponBase::PlayPendingAttackEffect()
{
	PlayAttackEffect(PendingAttackEffectLocation, PendingAttackEffectRotation);
	PlayAttackSound(PendingAttackEffectLocation, PendingAttackEffectRotation);
}

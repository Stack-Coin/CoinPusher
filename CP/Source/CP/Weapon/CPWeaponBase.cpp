// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPWeaponAnimationData.h"
#include "Weapon/CPAimDirectionInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
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

	if (AnimationData && AnimationData->AttackMontage)
	{
		if (ACharacter* OwnerCharacter = GetOwningCharacter())
		{
			const float MontageLength = OwnerCharacter->PlayAnimMontage(AnimationData->AttackMontage, 1.0f); //GetFinalAttackSpeed()
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
				AnimInstance->Montage_SetEndDelegate(EndDelegate, AnimationData->AttackMontage);
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
	bIsAttacking = false;

	if (AnimationData && AnimationData->AttackMontage)
	{
		if (ACharacter* OwnerCharacter = GetOwningCharacter())
		{
			OwnerCharacter->StopAnimMontage(AnimationData->AttackMontage);
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

	return WielderAttackPower + WeaponData.AttackPower;
}

float ACPWeaponBase::GetFinalAttackSpeed() const
{
	float WielderAttackSpeed = 1.0f;
	if (const ICPStatInterface* StatInterface = GetOwnerStatInterface())
	{
		WielderAttackSpeed = StatInterface->GetStat(ECPStatType::AttackSpeed);
	}

	return FMath::Max(WielderAttackSpeed * WeaponData.AttackSpeed, KINDA_SMALL_NUMBER);
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

void ACPWeaponBase::PlayAttackEffect(const FVector& Location) const
{
	if (WeaponData.AttackEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, WeaponData.AttackEffect, Location);
	}
}

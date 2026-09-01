// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPWeaponBase.h"
#include "Weapon/CPWeaponAnimationData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/CPStatInterface.h"
#include "Player/CPStatTypes.h"
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

	if (AnimationData && AnimationData->AttackMontage)
	{
		if (ACharacter* OwnerCharacter = GetOwningCharacter())
		{
			OwnerCharacter->PlayAnimMontage(AnimationData->AttackMontage, GetFinalAttackSpeed());
		}
	}

	PerformComboStep();
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

void ACPWeaponBase::PlayAttackEffect(const FVector& Location) const
{
	if (WeaponData.AttackEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, WeaponData.AttackEffect, Location);
	}
}

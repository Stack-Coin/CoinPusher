// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/CPWeaponManagerComponent.h"
#include "Weapon/CPWeaponBase.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UCPWeaponManagerComponent::UCPWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCPWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultWeaponClass)
	{
		EquipWeapon(DefaultWeaponClass);
	}
}

ACPWeaponBase* UCPWeaponManagerComponent::EquipWeapon(TSubclassOf<ACPWeaponBase> WeaponClass)
{
	if (!WeaponClass)
	{
		UnequipWeapon();
		return nullptr;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		return nullptr;
	}

	if (CurrentWeapon)
	{
		UnequipWeapon();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACPWeaponBase* NewWeapon = World->SpawnActor<ACPWeaponBase>(WeaponClass, OwnerCharacter->GetActorTransform(), SpawnParams);
	if (!NewWeapon)
	{
		return nullptr;
	}

	NewWeapon->EquipTo(OwnerCharacter, WeaponHandSocketName);
	CurrentWeapon = NewWeapon;

	OnWeaponChanged.Broadcast(CurrentWeapon);

	return CurrentWeapon;
}

void UCPWeaponManagerComponent::UnequipWeapon()
{
	if (!CurrentWeapon)
	{
		return;
	}

	ACPWeaponBase* OldWeapon = CurrentWeapon;
	CurrentWeapon = nullptr;

	OldWeapon->Unequip();
	OldWeapon->Destroy();

	OnWeaponChanged.Broadcast(nullptr);
}

ACPWeaponBase* UCPWeaponManagerComponent::SwapWeapon(TSubclassOf<ACPWeaponBase> NewWeaponClass)
{
	return EquipWeapon(NewWeaponClass);
}

bool UCPWeaponManagerComponent::Attack()
{
	if (!CurrentWeapon || !CurrentWeapon->CanAttack())
	{
		return false;
	}

	CurrentWeapon->Attack();
	return true;
}

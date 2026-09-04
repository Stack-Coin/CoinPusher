// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TestActor/CPHealthBarTestActor.h"
#include "UI/CPViewportHealthBarComponent.h"
#include "TimerManager.h"

ACPHealthBarTestActor::ACPHealthBarTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ViewportHealthBar = CreateDefaultSubobject<UCPViewportHealthBarComponent>(TEXT("ViewportHealthBar"));
}

void ACPHealthBarTestActor::BeginPlay()
{
	Super::BeginPlay();

	ResetTestHealth();

	if (DamageInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &ACPHealthBarTestActor::HandleDamageTick, DamageInterval, true);
	}
}

void ACPHealthBarTestActor::HandleDamageTick()
{
	if (CurrentHealth <= 0.0f)
	{
		ResetTestHealth();
		return;
	}

	TakeTestDamage(DamageAmount);
}

void ACPHealthBarTestActor::TakeTestDamage(float Amount)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.0f, MaxHealth);

	if (ViewportHealthBar)
	{
		ViewportHealthBar->UpdateHealth(CurrentHealth, MaxHealth);
	}
}

void ACPHealthBarTestActor::ResetTestHealth()
{
	CurrentHealth = MaxHealth;

	if (ViewportHealthBar)
	{
		ViewportHealthBar->UpdateHealth(CurrentHealth, MaxHealth);
	}
}
